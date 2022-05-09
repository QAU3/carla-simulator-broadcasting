// Fill out your copyright notice in the Description page of Project Settings.


#include "CarlaHUD.h"

// Sets default values
ACarlaHUD::ACarlaHUD()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// in your object constructor create the text object
	

}


// Called when the game starts or when spawned
void ACarlaHUD::BeginPlay()
{
	Super::BeginPlay();
	//listener = new TCPSocketListener(receivePort);

	
	
}

// Called every frame
void ACarlaHUD::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	

}

