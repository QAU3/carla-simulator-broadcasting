// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/UMG/Public/UMG.h"
#include "DataWidget.generated.h"

/**
 * 
 */
UCLASS()
class FRONTENDAPP_API UDataWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UDataWidget(const FObjectInitializer& ObjectInitializer);
	virtual void NativeConstruct() override;
	void UpdateDataView(FString data);
	void ResetView();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget));
	class UTextBlock* TXTData; 
};
