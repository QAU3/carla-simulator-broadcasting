/* The content of this program is ment to be used 
in the Console app <Managed NDIlib Send> provided in the NDI SDK Examples.sln 
You can find documenation and more stuff in C:\Program Files\NDI\NDI 5 SDK
When it was tested locally there was no significant latency, 

Script modified by QAU
Tested with NDI plugin in UE4.27

===== The current program ====
TCPImageReceiver:
*Handles the connectioin using TCP protocol. 
*It comes with an EventHandler that notifies the subscribers
every time an image arrives
*Also converts the byte array to an Image object

NDIImageSender:
*Handles the broadcasting using NDI SDK
*Its prepared to send image an audio at the same time 
but currently just sends images every time a complete image arrives via tcp socket

There are still things to be done:
0. OPTIMIZE OF COURSE
1. Exception thrown: 'System.OutOfMemoryException' in System.Drawing.dll
An unhandled exception of type 'System.OutOfMemoryException' occurred in System.Drawing.dll
That occurs whe drawing the image in the canvas
2. Exit and clean memmory
*/
using NewTek;
using NewTek.NDI;
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;

//QAU
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Drawing.Imaging;

namespace Managed_NDIlib_Send_Example
{
    class TCPImageEventArgs : EventArgs
    {
        public Image TCPImage { get; set; }
    }
    class TCPImageReceiver
    {
        public delegate void TCPImageReceivedEventHandler(object source, TCPImageEventArgs args);
        public event TCPImageReceivedEventHandler TCPImageReceived;

        private Thread listenThread;
        TcpListener server = null;

        string IP;
        int PORT;

        public string Ip { get => IP; set => IP = value; }
        public int Port { get => PORT; set => PORT = value; }

        public TCPImageReceiver(string ip, int port)
        {
            IP = ip;
            PORT = port;
        }
        protected virtual void OnTCPImageReceived(Image bImage)
        {
            if (TCPImageReceived != null)
            {
                TCPImageReceived(this, new TCPImageEventArgs() { TCPImage = bImage });
            }
        }
        public void Destroy()
        {
            server.Stop();
            if (this.listenThread.IsAlive)
            {
                this.listenThread.Abort();
            }
        }
        public void StartListener()
        {
            this.listenThread = new Thread(new ThreadStart(Listen));
            this.listenThread.IsBackground = true;
            this.listenThread.Start();
        }
        public Image ByteArrayToImage(byte[] byteArrayIn)
        {
            Image returnImage = null;
            using (MemoryStream ms = new MemoryStream(byteArrayIn))
            {
                returnImage = Image.FromStream(ms);
            }
            return returnImage;
        }

        public void Listen()
        {
            try
            {
                // Set the TcpListener on port 13000.
                Int32 port = this.PORT;
                IPAddress localAddr = IPAddress.Parse(this.IP);

                // TcpListener server = new TcpListener(port);
                server = new TcpListener(localAddr, port);

                // Start listening for client requests.
                server.Start();

                // Buffer for reading data
                Byte[] bytes = new Byte[1920 * 1080 * 4];
                
                Image img = null;
                // Enter the listening loop.
                while (true)
                {
                    Console.Write("Waiting for a connection... ");

                    // Perform a blocking call to accept requests.
                    // You could also use server.AcceptSocket() here.
                    TcpClient client = server.AcceptTcpClient();
                    Console.WriteLine("Connected!");

                  
                    img = null;
                    // Get a stream object for reading and writing
                    NetworkStream stream = client.GetStream();

                    int i;

                    // Loop to receive all the data sent by the client.
                    while ((i = stream.Read(bytes, 0, bytes.Length)) != 0)
                    {

                        // Console.WriteLine("Received in Server: {0}", bytes.Length);

                        //QAI onvert bytes to Image
                        img = ByteArrayToImage(bytes);
                        //INVOKE QAU CUSTOM EVENT
                        OnTCPImageReceived(img);

                        ///IF RESPONSE TO CLIENT NEEDED
                        // Process the data sent by the client.
                        //byte[] msg = System.Text.Encoding.ASCII.GetBytes(data);
                        // Send back a response.
                        //stream.Write(msg, 0, msg.Length);
                        //Console.WriteLine("Sent: {0}", data);
                    }

                    // Shutdown and end connection
                    client.Close();
                }
            }
            catch (SocketException e)
            {
                Console.WriteLine("SocketException: {0}", e);
            }
        }
    }

    class NDIImageSender
    {
        Bitmap bmp;
        Graphics graphics;
        IntPtr sendInstancePtr;
        NDIlib.audio_frame_v2_t audioFrame;
        IntPtr bufferPtr;

        int frameNumber;

        //Constructor
        public NDIImageSender() { }
        static void DrawPrettyText(Graphics graphics, String text, float size, FontFamily family, Point origin, StringFormat format, Brush fill, Pen outline)
        {
            // make a text path
            GraphicsPath path = new GraphicsPath();
            path.AddString(text, family, 0, size, origin, format);

            // Draw the pretty text
            graphics.FillPath(fill, path);
            graphics.DrawPath(outline, path);
        }

        // Because 48kHz audio actually involves 1601.6 samples per frame at 29.97fps, we make a basic sequence that we follow.
        static int[] audioNumSamples = { 1602, 1601, 1602, 1601, 1602 };

        // fills the audio buffer with a test tone or silence
        static void FillAudioBuffer(NDIlib.audio_frame_v2_t audioFrame, bool doTone)
        {
            // should never happen
            if (audioFrame.p_data == IntPtr.Zero)
                return;

            // temp space for floats
            float[] floatBuffer = new float[audioFrame.no_samples];

            // make the tone or silence
            double cycleLength = (double)audioFrame.sample_rate / 1000.0;
            int sampleNumber = 0;
            for (int i = 0; i < audioFrame.no_samples; i++)
            {
                double time = sampleNumber++ / cycleLength;
                floatBuffer[i] = doTone ? (float)(Math.Sin(2.0f * Math.PI * time) * 0.1) : 0.0f;
            }

            // fill each channel with our floats...
            for (int ch = 0; ch < audioFrame.no_channels; ch++)
            {
                // scary pointer math ahead...
                // where does this channel start in the unmanaged buffer?
                IntPtr destStart = new IntPtr(audioFrame.p_data.ToInt64() + (ch * audioFrame.channel_stride_in_bytes));

                // copy the float array into the channel
                Marshal.Copy(floatBuffer, 0, destStart, (int)audioFrame.no_samples);
            }
        }

        public void BroadCastImage(Image remoteImage)
        {
            ///QAU image
            ///
            graphics = null;
            frameNumber++;

            // define our bitmap properties
            int xres = 1920;
            int yres = 1080;
            int stride = (xres * 32/*BGRA bpp*/ + 7) / 8;
            int bufferSize = yres * stride;

            

            // allocate some memory for a video buffer
            bufferPtr = Marshal.AllocHGlobal((int)bufferSize);
            
            
            // We are going to create a 1920x1080 progressive frame at 29.97Hz.
            NDIlib.video_frame_v2_t videoFrame = new NDIlib.video_frame_v2_t()
            {
                // Resolution
                xres = xres,
                yres = yres,
                // Use BGRA video
                FourCC = NDIlib.FourCC_type_e.FourCC_type_BGRA,
                // The frame-eate
                frame_rate_N = 30000,
                frame_rate_D = 1001,
                // The aspect ratio (16:9)
                picture_aspect_ratio = (16.0f / 9.0f),
                // This is a progressive frame
                frame_format_type = NDIlib.frame_format_type_e.frame_format_type_progressive,
                // Timecode.
                timecode = NDIlib.send_timecode_synthesize,
                // The video memory used for this frame
                p_data = bufferPtr,
                // The line to line stride of this image
                line_stride_in_bytes = stride,
                // no metadata
                p_metadata = IntPtr.Zero,
                // only valid on received frames
                timestamp = 0
            };

            // set up an audio frame
            audioFrame = new NDIlib.audio_frame_v2_t()
            {
                // 48kHz in our example
                sample_rate = 48000,
                // Lets submit stereo although there is nothing limiting us
                no_channels = 2,
                // There can be up to 1602 samples at 29.97fps, we'll change this on the fly
                no_samples = 1602,
                // Timecode (synthesized for us !)
                timecode = NDIlib.send_timecode_synthesize,
                // The inter channel stride - this will also change on the fly
                channel_stride_in_bytes = sizeof(float) * 1602,
                // no metadata
                p_metadata = IntPtr.Zero,
                // only valid on received frames
                timestamp = 0

            };

            // allocate some unmanaged memory for an audio buffer
            // we're allocating more than needed for any number of samples we might need
            audioFrame.p_data = Marshal.AllocHGlobal((int)audioFrame.no_channels * 2000 * sizeof(float));

            // get a compatible bitmap and graphics context
            bmp = new Bitmap((int)xres, (int)yres, (int)stride, System.Drawing.Imaging.PixelFormat.Format32bppPArgb, bufferPtr);
            graphics = Graphics.FromImage(bmp);
            graphics.SmoothingMode = SmoothingMode.AntiAlias;

            
            // We'll use these later inside the loop
            StringFormat textFormat = new StringFormat();
            textFormat.Alignment = StringAlignment.Center;
            textFormat.LineAlignment = StringAlignment.Center;

            FontFamily fontFamily = new FontFamily("Arial");
            Pen outlinePen = new Pen(Color.Black, 2.0f);
            Pen thinOutlinePen = new Pen(Color.Black, 1.0f);

            // We will send 10000 frames of video.
            //for (int frameNumber = 0; frameNumber < 1000; frameNumber++)
            //{
                // are we connected to anyone?
                if (NDIlib.send_get_no_connections(sendInstancePtr, 1000) < 1)
                {
                    // no point rendering
                    Console.WriteLine("No current connections, so no rendering needed.");

                    // Wait a bit, otherwise our limited example will end before you can connect to it
                    System.Threading.Thread.Sleep(50);
                }
                else
                {
                            
                    ///IF AUDIOI IS NEEDED UNCOMMENT
                    // Because we are clocking to the video it is better to always submit the audio
                    // before, although there is very little in it. I'll leave it as an excercise for the
                    // reader to work out why.
                    //audioFrame.no_samples = audioNumSamples[frameNumber % 5];
                    //audioFrame.channel_stride_in_bytes = audioFrame.no_samples * sizeof(float);

                    // put tone in it every 30 frames
                    //bool doTone = frameNumber % 30 == 0;
                    //FillAudioBuffer(audioFrame, doTone);

                    // Submit the audio buffer
                    //NDIlib.send_send_audio_v2(sendInstancePtr, ref audioFrame);

                    // fill it with a lovely color
                    //graphics.Clear(Color.BlueViolet);

                    //QUA drawing the remote image
                    graphics.DrawImage(remoteImage, new Point(0, 0));

                    // show which source we are
                    DrawPrettyText(graphics, "C# NDIlib Example Source", 96.0f, fontFamily, new Point(960, 100), textFormat, Brushes.White, outlinePen);

                // Get the tally state of this source (we poll it),
                    NDIlib.tally_t NDI_tally = new NDIlib.tally_t();
                    NDIlib.send_get_tally(sendInstancePtr, ref NDI_tally, 0);

                    // Do something different depending on where we are shown
                    if (NDI_tally.on_program)
                        DrawPrettyText(graphics, "On Program", 96.0f, fontFamily, new Point(960, 225), textFormat, Brushes.White, outlinePen);
                    else if (NDI_tally.on_preview)
                        DrawPrettyText(graphics, "On Preview", 96.0f, fontFamily, new Point(960, 225), textFormat, Brushes.White, outlinePen);

                    //// show what frame we've rendered
                    DrawPrettyText(graphics, String.Format("Frame {0}", frameNumber.ToString()), 96.0f, fontFamily, new Point(960, 350), textFormat, Brushes.White, outlinePen);

                    // show current time
                    DrawPrettyText(graphics, System.DateTime.Now.ToString(), 96.0f, fontFamily, new Point(960, 900), textFormat, Brushes.White, outlinePen);

                    // We now submit the frame. Note that this call will be clocked so that we end up submitting 
                    // at exactly 29.97fps.
                    NDIlib.send_send_video_v2(sendInstancePtr, ref videoFrame);

                    // Just display something helpful in the console
                    Console.WriteLine("Frame number {0} sent.", frameNumber);

                    // Dispose of our graphics resources
                    graphics.Dispose();
                    bmp.Dispose();

                    // free our buffers
                    Marshal.FreeHGlobal(bufferPtr);
                    Marshal.FreeHGlobal(audioFrame.p_data);

               
                }
            //}//ENDOF FOR LOOP


        }
        public bool StartNDISender() 
        {
            // .Net interop doesn't handle UTF-8 strings, so do it manually
            // These must be freed later
            IntPtr sourceNamePtr = UTF.StringToUtf8("NDIlib Send Example");
            IntPtr groupsNamePtr = IntPtr.Zero;

            // Not required, but "correct". (see the SDK documentation)
            if (!NDIlib.initialize())
            {
                // Cannot run NDI. Most likely because the CPU is not sufficient (see SDK documentation).
                // you can check this directly with a call to NDIlib_is_supported_CPU()
                Console.WriteLine("Cannot run NDI");
                return false;
            }

            // Create an NDI source description using sourceNamePtr and it's clocked to the video.
            NDIlib.send_create_t createDesc = new NDIlib.send_create_t()
            {
                p_ndi_name = sourceNamePtr,
                p_groups = groupsNamePtr,
                clock_video = true,
                clock_audio = false
            };

            // We create the NDI finder instance
            sendInstancePtr = NDIlib.send_create(ref createDesc);

            // free the strings we allocated
            Marshal.FreeHGlobal(sourceNamePtr);
            Marshal.FreeHGlobal(groupsNamePtr);

            // did it succeed?
            if (sendInstancePtr == IntPtr.Zero)
            {
                Console.WriteLine("Failed to create send instance");
                return false;
            }
            return true;
        }

        public void Destroy() 
        {
           
            // Destroy the NDI sender
            NDIlib.send_destroy(sendInstancePtr);

            // Not required, but "correct". (see the SDK documentation)
            NDIlib.destroy();

        }
        public void OmImageReceived(Object source, TCPImageEventArgs e)
        {
            //Console.WriteLine("Received in ImageRender:");
            BroadCastImage(e.TCPImage);
        }


    }
    class Program
    {
        static TCPImageReceiver imageReceiver;
        static NDIImageSender imageSender;
       

        static void Main()
        {
            //QAU
            imageReceiver = new TCPImageReceiver("127.0.0.1", 2337);
            imageSender = new NDIImageSender();

            imageReceiver.TCPImageReceived += imageSender.OmImageReceived;

            if (!imageSender.StartNDISender()) 
            {
                Console.WriteLine("Error--> NDI SENDER NOT CREATED");
            }
            
            imageReceiver.StartListener();
            

            ///QAU
            Console.WriteLine("\nHit;enter to continue...");
            Console.Read();
            imageReceiver.Destroy();
            imageSender.Destroy();
        }
    }
}
