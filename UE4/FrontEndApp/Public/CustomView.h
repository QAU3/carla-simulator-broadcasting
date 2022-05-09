// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Runtime/Sockets/Public/Sockets.h"
#include "Runtime/Sockets/Public/SocketSubsystem.h"
#include "Runtime/Networking/Public/Interfaces/IPv4/IPv4Address.h"
#include "../CarlaStreamingCommon.h"

#include "CustomView.generated.h"

UCLASS()
class FRONTENDAPP_API ACustomView : public AActor
{
	GENERATED_BODY()
private:
	TCPSocketListener* listener;
	TCPPacketReceiver* packetReceiver;
public:	
	// Sets default values for this actor's properties
	ACustomView();
	virtual void BeginDestroy();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	uint8* lastReceivedPacket;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int receivePort = 2338;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
