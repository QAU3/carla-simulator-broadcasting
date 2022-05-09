// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Components/WidgetComponent.h"
#include "DataWidget.h"
#include "CarlaUI.generated.h"

/**
 * 
 */
UCLASS()
class FRONTENDAPP_API ACarlaUI : public AHUD
{
	GENERATED_BODY()
public:
	ACarlaUI();
	virtual void DrawHUD() override; 
	virtual void BeginPlay() override; 
	virtual void Tick(float DeltaSeconds) override; 

	UFUNCTION()
	void UpdateData(FString data);

	UFUNCTION()
	void ResetData();

	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
		TSubclassOf<UUserWidget> DataWidgetClass;
private:
	UDataWidget* DataWidget;
};
