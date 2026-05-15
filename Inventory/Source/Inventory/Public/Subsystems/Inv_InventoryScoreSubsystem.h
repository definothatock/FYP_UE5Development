// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Inv_InventoryScoreSubsystem.generated.h"

class APlayerController;

/** Fired on the server immediately after a successful ClearInventoryAndSaveScore commit. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FInvOnInventoryClearedScoreCommitted,
	APlayerController*, PlayerController,
	int32, ClearedScoreThisSubmit,
	int32, PlayerAccumulatedClearedScore);

UCLASS()
class INVENTORY_API UInv_InventoryScoreSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Score")
	FInvOnInventoryClearedScoreCommitted OnClearedScoreCommitted;

	UFUNCTION(BlueprintPure, Category = "Inventory|Score", meta = (WorldContext = "WorldContextObject"))
	static UInv_InventoryScoreSubsystem* Get(const UObject* WorldContextObject);

	void BroadcastClearedScoreCommitted(APlayerController* PlayerController, int32 ClearedScoreThisSubmit,
	                                    int32 PlayerAccumulatedClearedScore);
};
