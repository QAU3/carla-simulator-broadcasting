// Fill out your copyright notice in the Description page of Project Settings.


#include "CustomView.h"

// Sets default values
ACustomView::ACustomView()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


}

// Called when the game starts or when spawned
void ACustomView::BeginPlay()
{
	Super::BeginPlay();

	/*listener = new TCPSocketListener(receivePort);
	packetReceiver = new TCPPacketReceiver(network_buffer_size, image_size);*/

	TArray<UTextRenderComponent*> components;

	GetComponents<UTextRenderComponent>(components);
	UE_LOG(LogTemp, Warning, TEXT("%d"), components.Num());
	if (components.Num() > 0)
	{
		UTextRenderComponent* textView = components[0];
		textView->SetText(FText::FromString("Hola, waiting for connection!!"));
	}

}

// Called every frame
void ACustomView::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	//listener->Update();

	//for (FSocket* client : listener->Connections)
	//{

	//	packetReceiver->ReceivePacket(client);
	//	lastReceivedPacket = packetReceiver->last_received_packet;

	//}

}

void ACustomView::BeginDestroy()
{
	Super::BeginDestroy();
}
