// Fill out your copyright notice in the Description page of Project Settings.


#include "CarlaStreamingCommon.h"

TArray<int32> PortsContainer::portList;

FSocket* CreateTCPListenSocket(int32 port)
{
	PortsContainer obj;
	//static TArray<int32> portList;
	static int32 _port;


	FSocket* Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);

	FIPv4Address ip(127,0,0,1);
	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	addr->SetIp(ip.Value);

	_port= GetNextUnbindedPort(Socket, addr, port);

	addr->SetPort(_port);
	//Socket->Bind(*addr);
	//Socket->Connect(*addr);
	if (!Socket->Bind(*addr))
	{

		UE_LOG(LogTemp, Error, TEXT("Error binding TCP Socket to port %d"), port);
		//if (portList.Contains(_port))
		//{
		//	portList.Sort();
		//	_port = portList[portList.Num() - 1] + 1;
		//}
	}
	//portList.Add(_port);
	//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Blue, TEXT("Passed: %d"), port);
	UE_LOG(LogTemp, Error, TEXT("Passed %d"), _port);
	obj.portList.Add(_port);


	Socket->SetNonBlocking(true);
	Socket->Listen(8);
	obj.Print();
	return Socket;
}


int32 GetNextUnbindedPort(FSocket* Socket, TSharedRef<FInternetAddr>  addr, int32 port)
{

	addr->SetPort(port);
	if (!Socket->Bind(*addr))
	{
		UE_LOG(LogTemp, Error, TEXT("Triying netx port %d"), port);
		return GetNextUnbindedPort(Socket,addr,port+1);
		
	}
	return port;
}

TCPPacketReceiver::TCPPacketReceiver(size_t network_buffer_size, size_t expected_packet_size)
{
	this->network_buffer_size = network_buffer_size;
	this->expected_packet_size = expected_packet_size;
	network_buffer = new uint8[network_buffer_size];
	packet_buffer = new uint8[expected_packet_size];
	last_received_packet = new uint8[expected_packet_size];
	total_received = 0;
}

TCPPacketReceiver::~TCPPacketReceiver()
{
	delete network_buffer;
	delete packet_buffer;
	delete last_received_packet;
}


bool TCPPacketReceiver::ReceivePacket(FSocket* client)
{
	bool packet_completed =true;
	
	int32 bytesRead = 0;
	client->Recv(network_buffer, network_buffer_size, bytesRead);

	return packet_completed;
}

TCPImageReceiver::TCPImageReceiver(size_t network_buffer_size, size_t expected_image_size)
{
	this->network_buffer_size = network_buffer_size;
	this->expected_image_size = expected_image_size;
	network_buffer = new uint8[network_buffer_size];
	image_buffer = new uint8[expected_image_size];
	last_received_image = new uint8[expected_image_size];
	total_received = 0;
}

TCPImageReceiver::~TCPImageReceiver()
{
	delete network_buffer;
	delete image_buffer;
	delete last_received_image;
}

bool TCPImageReceiver::ReceiveImage(FSocket* client, uint32* optional_output_texture)
{
	bool image_completed = false;
	uint32 dataSize;
	while (client->HasPendingData(dataSize))
	{
		int32 receivedCount;

		client->Recv(network_buffer, network_buffer_size, receivedCount);
		//client->Recv(imageReceiveBuffer+totalReceived, dataSize, receivedCount);

		total_received += receivedCount;
		//UE_LOG(LogTemp, Warning, TEXT("received %d bytes, total received: %d"), receivedCount, totalReceived);
		if (total_received < expected_image_size)
		{
			FMemory::Memcpy(image_buffer + total_received - receivedCount, network_buffer, receivedCount);
		}
		else
		{
			uint32 next_image_data_size = total_received - expected_image_size;
			uint32 current_image_data_size = receivedCount - next_image_data_size;
			FMemory::Memcpy(image_buffer + total_received - receivedCount, network_buffer, current_image_data_size);

			//UE_LOG(LogTemp, Warning, TEXT("full image received: %d bytes: remaining %d"), totalReceived, next_image_data_size);

			if (optional_output_texture != nullptr)
			{
				FMemory::Memcpy(optional_output_texture, image_buffer, expected_image_size);

				uint8* tmp = image_buffer;
				image_buffer = last_received_image;
				last_received_image = tmp;
			}

			FMemory::Memcpy(image_buffer, network_buffer + current_image_data_size, next_image_data_size);
			total_received = next_image_data_size;
			image_completed = true;
		}
	}

	return image_completed;
}

TCPSocketListener::TCPSocketListener(int32 port)
{
	_port = port;
	Socket = CreateTCPListenSocket(port);
}

TCPSocketListener::~TCPSocketListener()
{
	if (Socket != nullptr)
	{
		if (!Socket->Close())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to close socket!"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Closed socket successfully!"));
		}

		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}

	for (FSocket* client : Connections)
	{
		if (client != nullptr)
		{
			client->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(client);
		}
	}
}

void TCPSocketListener::Update()
{
	bool hasPendingConnection;
	Socket->HasPendingConnection(hasPendingConnection);
	if (hasPendingConnection)
	{
		TSharedPtr<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		FSocket* connection = Socket->Accept(*addr, TEXT("debug description??"));
		Connections.push_back(connection);
	}

	for (int i = Connections.size() - 1; i >= 0; --i)
	{
		FSocket* client = Connections[i];
		if (client->GetConnectionState() != ESocketConnectionState::SCS_Connected)
		{
			Connections[i] = Connections[Connections.size() - 1];
			Connections.pop_back();
		}
	}
}