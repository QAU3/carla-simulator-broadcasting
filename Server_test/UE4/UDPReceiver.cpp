// Fill out your copyright notice in the Description page of Project Settings.


#include "UDPReceiver.h"
#include <cmath>

#include "..\Public\UDPReceiver.h"

constexpr int image_width = 1920;
constexpr int image_height = 1080;
constexpr int bytes_per_pixel = 4;
constexpr int bytes_per_pixel_depth = 4;
constexpr int image_size = image_width * image_height * bytes_per_pixel;
constexpr int image_size_depth = image_width * image_height * bytes_per_pixel_depth;
constexpr int network_buffer_size = 0b1111111111111111;
int camTextureIndex = 0;
uint8* img_buff = new uint8[image_size];



int total_received = 0;
const int EXPECTED_PACKETS = 127;

// Sets default values
AUDPReceiver::AUDPReceiver()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUDPReceiver::BeginPlay()
{
	Super::BeginPlay();
	StartUDPReceiver();
	StartUDPSender("UDPSender", "127.0.0.1",2338);

	FName texName(FString::Printf(TEXT("cam_texture_%d"), camTextureIndex));
	camTextureIndex++;

	//camTexture = UTexture2D::CreateTransient(image_width, image_height, EPixelFormat::PF_A8R8G8B8, texName);

	camTexture = UTexture2D::CreateTransient(image_width, image_height , EPixelFormat::PF_B8G8R8A8, texName);
	//camTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

	//for some reason, the texture doesn't get overriden later on if we don't overwrite it's data once at the start here
	uint32* dest = (uint32*)camTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

	uint8* buf = new uint8[image_size];
	for (int i = 0; i < image_size; i += 4)
	{
		buf[i] = 255;
		buf[i + 1] = 255;
		buf[i + 2] = 0;
		buf[i + 3] = 255;
	}

	FMemory::Memcpy(dest, buf, image_size);

	camTexture->PlatformData->Mips[0].BulkData.Unlock();
	camTexture->UpdateResource();
	delete[] buf;

	TArray<UStaticMeshComponent*> components;
	GetComponents<UStaticMeshComponent>(components);
	if (components.Num() > 0)
	{
		UStaticMeshComponent* mesh = components[0];
		Plane = mesh;
		UMaterialInterface* mat = mesh->GetMaterial(0);

		UMaterialInstanceDynamic* dynamicMat = mesh->CreateAndSetMaterialInstanceDynamic(0);
		mesh->SetMaterial(0, dynamicMat);
		//dynamicMat->SetVectorParameterValue("Base Color", FLinearColor(2.0f, 0.1f, 0.1f, 1.0f));
		dynamicMat->SetTextureParameterValue(TEXT("Texture"), camTexture);
	}

}
void AUDPReceiver::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	//~~~~~~~~~~~~~~~~
	if (camTexture != nullptr) camTexture->ConditionalBeginDestroy();

	delete UDPReceiver;
	UDPReceiver = nullptr;

	//Clear all sockets!
	//		makes sure repeat plays in Editor dont hold on to old sockets!
	if (ListenSocket)
	{
		ListenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
	}
	if (SenderSocket)
	{
		SenderSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(SenderSocket);
	}
}

bool AUDPReceiver::SegmentReceived(uint8 Segment) 
{
	return true;
}

void AUDPReceiver::SendData(FArrayWriter Data2Send, int32 DataSize)
{
	SenderSocket->SendTo(Data2Send.GetData(), Data2Send.Num(), DataSize, *RemoteAddr);
	if (DataSize <= 0)
	{
		const FString Str = "Socket is valid but the receiver received 0 bytes, make sure it is listening properly!";
		UE_LOG(LogTemp, Error, TEXT("%s"), *Str);
	}
}

void AUDPReceiver::OnDataReceivedDelagated(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt)
{
	//char* Data = (char*)ArrayReaderPtr->GetData();
	//FString Msg = FString(ANSI_TO_TCHAR(Data));
	uint8* Data = ArrayReaderPtr->GetData();

	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Data received: %d"), Data[0]));
	
	int bytesReceived = ArrayReaderPtr->Num();
	int img_received_count = bytesReceived - 1;
	//Send_ACK((int) Data[0], bytesReceived, &Data[1]);

	int FLAG = 0;
	int Segment = Data[0];

	UE_LOG(LogTemp, Warning, TEXT("Arrived %d"), Segment);
	int32 BytesSent = 0;
	FArrayWriter Writer;
	if (SegmentsArrived.Num() == EXPECTED_PACKETS-1)
	{
		total_received = 0;
		SegmentsArrived.Empty();

		uint32* dest = (uint32*)camTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(dest, img_buff, image_size);

		camTexture->PlatformData->Mips[0].BulkData.Unlock();
		camTexture->UpdateResource();
		total_received = 0;
		img_buff = new uint8[image_size];

	}

	if (SegmentsArrived.Contains(Segment))
	{
		UE_LOG(LogTemp, Error, TEXT("SEGMENT ALREADY THERE %d"), Segment);

		Writer << FLAG;
		SendData(Writer, BytesSent);
	}
	else
	{
		total_received += (bytesReceived - 1);

		FLAG = 1;
		SegmentsArrived.Add(Segment);
		UE_LOG(LogTemp, Warning, TEXT("SEGMENT ADDED %d"), Segment);

		Writer << Segment;
		SendData(Writer, BytesSent);
		//COPY IN MEMORY
		FMemory::Memcpy(img_buff + total_received - img_received_count, &Data[1], img_received_count);

	}

}

void AUDPReceiver::Send_ACK(int Segment, int bytesReceived, uint8* PaketReceived) {
	

}



void AUDPReceiver::StartUDPReceiver()
{
	int32 port=6000;
	FIPv4Address Addr;
	FIPv4Address::Parse(TEXT("0.0.0.0"), Addr);


	FIPv4Address Group;
	FIPv4Address::Parse(TEXT("224.1.1.1"), Group);

	//Create Socket
	FIPv4Endpoint Endpoint(Addr, port);
	FIPv4Endpoint AnyEndpoint(FIPv4Address::Any,port);

	//BUFFER SIZE
	int32 BufferSize = FMath::Pow(2,16);

	ListenSocket = FUdpSocketBuilder("UDPSender")
		.WithMulticastLoopback()
		.WithMulticastTtl(2)
		//.WithBroadcast()
		.JoinedToGroup(Group)
		.AsNonBlocking()
		.AsReusable()
		//.BoundToEndpoint(AnyEndpoint)
		//.BoundToAddress(Addr)
		.BoundToPort(port)
		.WithReceiveBufferSize(BufferSize)
		.Build();


	if (ListenSocket != nullptr) 
	{
		UE_LOG(LogTemp, Warning, TEXT("Success creating socket!!"));

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, FString::Printf(TEXT("Success: %d"), port));

		FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
		UDPReceiver = new FUdpSocketReceiver(ListenSocket, ThreadWaitTime, TEXT("MyThread"));

		UDPReceiver->OnDataReceived().BindUObject(this, &AUDPReceiver::OnDataReceivedDelagated);
		//OnReceiveSocketStartedListening.Broadcast();

		UDPReceiver->Start();
	}
	else 
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create socket!!"));

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("Faildet to create socket: %d"), port));

	}


}

bool AUDPReceiver::StartUDPSender(const FString& YourChosenSocketName, const FString& TheIP, const int32 ThePort)
{
//Create Remote Address.
	RemoteAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

	bool bIsValid;
	RemoteAddr->SetIp(*TheIP, bIsValid);
	RemoteAddr->SetPort(ThePort);

	if (!bIsValid)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("NON VALID ADDR: %s:%d"), *TheIP,ThePort));
		return false;
	}

	SenderSocket = FUdpSocketBuilder(*YourChosenSocketName)
		.AsReusable()
		.WithBroadcast();

	//Set Send Buffer Size
	int32 SendSize = 1024;
	SenderSocket->SetSendBufferSize(SendSize, SendSize);
	SenderSocket->SetReceiveBufferSize(SendSize, SendSize);

	UE_LOG(LogTemp, Log, TEXT("\n\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"));
	UE_LOG(LogTemp, Log, TEXT("Rama ****UDP**** Sender Initialized Successfully!!!"));
	UE_LOG(LogTemp, Log, TEXT("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\n"));

	return true;
}

// Called every frame
void AUDPReceiver::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}




/*if (SegmentsArrived.Num() > 0)
{
	for (int32 i=0; i < SegmentsArrived.Num(); i++)
	{
		UE_LOG(LogTemp, Error, TEXT("Ele %d"), SegmentsArrived[i])
	}
}
else
{
	UE_LOG(LogTemp,Error,TEXT("Empty array %d"), SegmentsArrived.Num())
}*/

//int bytesReceived = ArrayReaderPtr->Num();
//total_received += (bytesReceived - 1);
//int img_received_count = bytesReceived - 1;
//
//++total_received;

//if (total_received < EXPECTED_PACKETS) 
//{
//	FMemory::Memcpy(img_buff+(total_received-1)* img_received_count,&Data[1],img_received_count);
//}
//else
//{
//	FMemory::Memcpy(img_buff + (total_received - 1) * img_received_count, &Data[1], img_received_count);

//	uint32* dest = (uint32*)camTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
//	FMemory::Memcpy(dest, img_buff, image_size);

//	camTexture->PlatformData->Mips[0].BulkData.Unlock();
//	camTexture->UpdateResource();
//	total_received = 0;
//	//img_buff = new uint8[image_size];
//}
//
/*if (Data[0] > 1)
{
	FMemory::Memcpy(img_buff + total_received - img_received_count, &Data[1], img_received_count);
}
else
{

 FMemory::Memcpy(img_buff + total_received - img_received_count, &Data[1], img_received_count);

	uint32* dest = (uint32*)camTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(dest, img_buff, image_size);

	camTexture->PlatformData->Mips[0].BulkData.Unlock();
	camTexture->UpdateResource();
	total_received = 0;
	img_buff = new uint8[image_size];
}*/