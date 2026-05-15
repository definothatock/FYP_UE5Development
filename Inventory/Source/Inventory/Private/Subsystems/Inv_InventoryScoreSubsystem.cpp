// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystems/Inv_InventoryScoreSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UInv_InventoryScoreSubsystem* UInv_InventoryScoreSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World || !World->GetGameInstance())
	{
		return nullptr;
	}

	return World->GetGameInstance()->GetSubsystem<UInv_InventoryScoreSubsystem>();
}

void UInv_InventoryScoreSubsystem::BroadcastClearedScoreCommitted(APlayerController* PlayerController,
                                                                  const int32 ClearedScoreThisSubmit,
                                                                  const int32 PlayerAccumulatedClearedScore)
{
	OnClearedScoreCommitted.Broadcast(PlayerController, ClearedScoreThisSubmit, PlayerAccumulatedClearedScore);
}
