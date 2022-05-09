// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Runtime/Networking/Public/Networking.h"
#include "Runtime/Sockets/Public/Sockets.h"

#include "Components/StaticMeshComponent.h"
#include "Runtime/Engine/Public/ImageUtils.h"

#include <vector>
#include "GameFramework/Actor.h"
#include "UDPReceiver.generated.h"




UCLASS()
class UPDCONNECTIONTEST_API AUDPReceiver : public AActor
{
	GENERATED_BODY()
	
public:	

	// Sets default values for this actor's properties
	AUDPReceiver();
	//Sender
	TSharedPtr<FInternetAddr>	RemoteAddr;
	FSocket* SenderSocket;
	bool IsUDP;
	TArray<int> SegmentsArrived;

	//Receiver 
	FSocket* ListenSocket;
	FUdpSocketReceiver* UDPReceiver = nullptr;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
		UStaticMeshComponent* Plane;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
		UTexture2D* camTexture;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	void OnDataReceivedDelagated(const FArrayReaderPtr& ArrayReaderPtr, const FIPv4Endpoint& EndPt);
	void Send_ACK(int Segment,int bytesReceived, uint8* PaketReceived);
	void StartUDPReceiver();
	
	bool StartUDPSender(const FString& YourChosenSocketName,const FString& TheIP,const int32 ThePort);

	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool SegmentReceived(uint8 Segment);

	void SendData(FArrayWriter Data2Send, int32 DataSize);


};
