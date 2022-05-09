// Fill out your copyright notice in the Description page of Project Settings.


#include "DataWidget.h"

UDataWidget::UDataWidget(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{

}

void UDataWidget::NativeConstruct() 
{
	Super::NativeConstruct();
}


void UDataWidget::UpdateDataView(FString  data)
{
	if (TXTData) 
	{
		if (TXTData->Visibility== ESlateVisibility::Hidden) {TXTData -> SetVisibility(ESlateVisibility::Visible);}
	}

	TXTData->SetText(FText::FromString(data));
}


void UDataWidget::ResetView() 
{
	if (TXTData)
	{
		TXTData -> SetVisibility(ESlateVisibility::Hidden);
	}
}