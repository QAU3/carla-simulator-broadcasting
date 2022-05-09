// Fill out your copyright notice in the Description page of Project Settings.


#include "CarlaUI.h"

ACarlaUI::ACarlaUI()
{
}


void ACarlaUI::DrawHUD()
{
	Super::DrawHUD();
}

void ACarlaUI::BeginPlay()
{
	Super::BeginPlay();
	if (DataWidgetClass) { 
		DataWidget = CreateWidget<UDataWidget>(GetWorld(), DataWidgetClass); 
		if (DataWidget) 
		{
			DataWidget->AddToViewport();
		}
	}
}

void ACarlaUI::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ACarlaUI::UpdateData(FString data)
{
	if (DataWidget) 
	{
		DataWidget->UpdateDataView(data);
	}
}

void ACarlaUI::ResetData()
{
	if (DataWidget)
	{
		DataWidget->ResetView();
	}
}
