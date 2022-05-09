// Fill out your copyright notice in the Description page of Project Settings.


#include "CarlaDepthReceiver.h"
#include "../CarlaStreamingCommon.h"

// Sets default values
ACarlaDepthReceiver::ACarlaDepthReceiver()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}


// Called when the game starts or when spawned
void ACarlaDepthReceiver::BeginPlay()
{
	Super::BeginPlay();

	listener = new TCPSocketListener(receivePort);
	imageReceiver = new TCPImageReceiver(network_buffer_size, image_size_depth);

	FName texName(FString::Printf(TEXT("cam_texture_%d"), camTextureIndex));
	camTextureIndex++;

	camTexture = UTexture2D::CreateTransient(1920, 1080, EPixelFormat::PF_R8G8B8A8, texName);
	camTexture->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;

	//for some reason, the texture doesn't get overriden later on if we don't overwrite it's data once at the start here
	uint32* dest = (uint32*)camTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

	uint8* buf = new uint8[image_size_depth];
	for (int i = 0; i < image_size_depth; i += 4)
	{
		buf[i] = 255;
		buf[i + 1] = 0;
		buf[i + 2] = 255;
		buf[i + 3] = 255;
	}

	FMemory::Memcpy(dest, buf, image_size_depth);

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

void ACarlaDepthReceiver::BeginDestroy()
{
	Super::BeginDestroy();

	if (camTexture != nullptr) camTexture->ConditionalBeginDestroy();
	delete listener;
	delete imageReceiver;
}

int count = 0;

//Called every frame
void ACarlaDepthReceiver::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	listener->Update();

	for (FSocket* client : listener->Connections)
	{
		uint32* dest = (uint32*)camTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);

		imageReceiver->ReceiveImage(client, dest);

		camTexture->PlatformData->Mips[0].BulkData.Unlock();
		camTexture->UpdateResource();

		lastReceivedImage = imageReceiver->last_received_image;
	}
}