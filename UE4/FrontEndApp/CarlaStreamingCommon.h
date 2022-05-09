// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include<iostream>
#include<fstream>
#include "CoreMinimal.h"
#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Sockets/Public/SocketSubsystem.h"
#include "Runtime/Networking/Public/Interfaces/IPv4/IPv4Endpoint.h"
#include "Runtime/Networking/Public/Interfaces/IPv4/IPv4Address.h"
#include "Runtime/Networking/Public/Common/TcpSocketBuilder.h"
#include <vector>



using std::cout; using std::ofstream;
using std::endl; using std::string;
using std::fstream;

constexpr int image_width = 1920;
constexpr int image_height = 1080;
constexpr int bytes_per_pixel = 4;
constexpr int bytes_per_pixel_depth = 4;
constexpr int image_size = image_width * image_height * bytes_per_pixel;
constexpr int image_size_depth = image_width * image_height * bytes_per_pixel_depth;
constexpr int network_buffer_size = 0b1111111111111111;
int camTextureIndex = 0;
 

constexpr uint8 segmentation_color_map[13][4] = {
	{0,0,0,0},
	{255,0,0,0},
	{0,255,0,0},
	{255,255,0,0},
	{0,0,255,0},
	{255,0,255,0},
	{0,255,255,0},
	{255,255,255,0},
	{80,80,80,0},
	{80,30,240,0},
	{240,125,45,0},
	{55,193,28,0},
	{255,125,255,0}
};

int32 GetNextUnbindedPort(FSocket* Socket, TSharedRef<FInternetAddr>  addr, int32 port);

FSocket* CreateTCPListenSocket(int32 port);

//listens for connection requests on a certain port and accepts client connections as they come in
//construct with given port and call Update() every frame. .Connections is a vector containing all the accepted client connection Sockets

class PortsContainer
{
public:
	static TArray<int32> portList;
	void Print() 
	{
	

		FString file = FPaths::ProjectConfigDir();
		file.Append(TEXT("PORTS_USED.txt"));
		IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
		TArray<FString> StringsToWrite;
		string filename(std::string(TCHAR_TO_UTF8(*file)));
		fstream file_out;
		file_out.open(filename, std::ios_base::out);

		for (int32 n : portList)
		{
			UE_LOG(LogTemp, Error, TEXT("item %d"), n);

			StringsToWrite.Add(FString(TEXT("%d"),n));
			if (!file_out.is_open()) {
				cout << "failed to open " << filename << '\n';
				UE_LOG(LogTemp, Error, TEXT("failed to open "));

			}
			else {
				UE_LOG(LogTemp, Error, TEXT("wrote!!"));
				file_out << n  << endl;
				cout << n << endl;
			}
			
		}

		//if (FileManager.FileExists(*file))
		//{
		//	// We use the LoadFileToString to load the file into
		//	if (FFileHelper::SaveStringArrayToFile(StringsToWrite, *file))
		//	{
		//		UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Sucsesfuly Written: \"%d\" FStrings to the text file"), StringsToWrite.Num());
		//	}
		//	else
		//	{
		//		UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Failed to write FString to file."));
		//	}
		//}
		//else
		//{
		//	UE_LOG(LogTemp, Warning, TEXT("FileManipulation: ERROR: Can not read file because it was not found."));
		//	UE_LOG(LogTemp, Warning, TEXT("FileManipulation: Expected file location: %s"), *file);
		//}


	}

	void Empty() 
	{
		portList.Empty();
	}
};

class TCPSocketListener
{
private:
	int _port;
	FSocket* Socket;
	

public:
	
	TCPSocketListener(int32 port);
	~TCPSocketListener();

	void Update();

	std::vector<FSocket*> Connections;
};

//receives images of a fixed size from a given Socket (via ReceiveImage, should be called every frame).
//last fully received image is available in last_received_image
class TCPImageReceiver
{
private:
	uint8* network_buffer;
	uint8* image_buffer;

	size_t network_buffer_size;
	size_t total_received;
	size_t expected_image_size;

public:
	uint8* last_received_image;

public:
	TCPImageReceiver(size_t network_buffer_size, size_t expected_image_size);
	~TCPImageReceiver();

	//returns true if an image was completed this frame (since parts of the image may be received over multiple frames), false otherwise
	//multiple images may be received in a single frame, too. in this case it will also just return true, indicating that at least once image has been completely received
	bool ReceiveImage(FSocket* client, uint32* optional_output_texture);
};

class TCPPacketReceiver
{
private:
	uint8* network_buffer;
	uint8* packet_buffer;

	size_t network_buffer_size;
	size_t total_received;
	size_t expected_packet_size;
public:
	uint8* last_received_packet;

public:
	TCPPacketReceiver(size_t network_buffer_size, size_t expected_image_size);
	~TCPPacketReceiver();

	//returns true if an image was completed this frame (since parts of the image may be received over multiple frames), false otherwise
	//multiple images may be received in a single frame, too. in this case it will also just return true, indicating that at least once image has been completely received
	bool ReceivePacket(FSocket* client);
};