// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/Inv_InfoMessage.h"

#include "Components/TextBlock.h"

void UInv_InfoMessage::SetMessageVisibility(const bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UInv_InfoMessage::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(Text_Message))
	{
		Text_Message->SetText(FText::GetEmpty());
	}

	SetMessageVisibility(false);
	MessageHide();
}

void UInv_InfoMessage::SetMessage(const FText& Message)
{
	if (IsValid(Text_Message))
	{
		Text_Message->SetText(Message);
	}

	if (!bIsMessageActive)
	{
		SetMessageVisibility(true);
		MessageShow();
	}
	bIsMessageActive = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(MessageTimer, [this]()
		{
			SetMessageVisibility(false);
			MessageHide();
			bIsMessageActive = false;
		}, MessageLifetime, false);
	}
}
