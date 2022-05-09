// Fill out your copyright notice in the Description page of Project Settings.


#include "CarlaSegmentationReceiver.h"
#include "../CarlaStreamingCommon.h"

// Sets default values
ACarlaSegmentationReceiver::ACarlaSegmentationReceiver()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ACarlaSegmentationReceiver::BeginPlay()
{
	Super::BeginPlay();

	listener = new TCPSocketListener(receivePort);
	imageReceiver = new TCPImageReceiver(network_buffer_size, image_size);

	FName texName(FString::Printf(TEXT("cam_texture_%d"), camTextureIndex));
	camTextureIndex++;

	camTexture = UTexture2D::CreateTransient(1920, 1080, EPixelFormat::PF_B8G8R8A8, texName);
	camTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

	//for some reason, the texture doesn't get overriden later on if we don't overwrite it's data once at the start here
	uint32* dest = (uint32*)camTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

	uint8* buf = new uint8[image_size];
	for (int i = 0; i < image_size; i += 4)
	{
		buf[i] = 255;
		buf[i + 1] = 0;
		buf[i + 2] = 255;
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

void ACarlaSegmentationReceiver::BeginDestroy()
{
	Super::BeginDestroy();

	if (camTexture != nullptr) camTexture->ConditionalBeginDestroy();
	delete listener;
	delete imageReceiver;
}

int count = 0;

// Called every frame
void ACarlaSegmentationReceiver::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	listener->Update();

	for (FSocket* client : listener->Connections)
	{
		uint32* dest = (uint32*)camTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

		if (imageReceiver->ReceiveImage(client, dest))
		{
			uint8* uint8_dest = (uint8*)dest;
			for (int i = 0; i < image_size; i += 4)
			{
				const uint8* color = segmentation_color_map[uint8_dest[i + 2]]; //the red channel contains the index of the segment type, see carla docs
				uint8_dest[i] = color[0];
				uint8_dest[i + 1] = color[1];
				uint8_dest[i + 2] = color[2];
				uint8_dest[i + 3] = color[3];
			}
		}

		camTexture->PlatformData->Mips[0].BulkData.Unlock();
		camTexture->UpdateResource();

		lastReceivedImage = imageReceiver->last_received_image;
	}
}
