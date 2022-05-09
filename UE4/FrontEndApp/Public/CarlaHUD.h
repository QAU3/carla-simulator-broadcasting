// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Actor.h"
#include "CarlaHUD.generated.h"
UCLASS()
class FRONTENDAPP_API ACarlaHUD : public AActor
{
	GENERATED_BODY()
	//TCPSocketListener* listener;
	//TCPImageReceiver* imageReceiver;
public:	
	// Sets default values for this actor's properties
	ACarlaHUD();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	//uint8* lastRecivedPacket;

	//UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
		//UTextRenderComponent* TextView;
	//UPROPERTY(BlueprintReadWrite, EditAnywhere)
		//int receivePort = 2338;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
