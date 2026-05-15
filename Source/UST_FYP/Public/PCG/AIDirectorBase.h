// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIDirectorBase.generated.h"

// ============================================================================
//                                  STRUCTS
// ============================================================================

// Defines a single spawn event entry configurable in the Details Panel
USTRUCT(BlueprintType)
struct FEventSpawnConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
    TSubclassOf<AActor> SpawnClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
    float MinIntensity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
    float MaxIntensity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
    float Weight = 1.0f;

    // Identifies which Custom Event to trigger (e.g. SpawnSporeBubble, SpawnEnemy)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
    FName EventName = NAME_None;
};

// ============================================================================
//                              AAIDIRECTORBASE
// ============================================================================

UCLASS()
class UST_FYP_API AAIDirectorBase : public AActor
{
    GENERATED_BODY()

public:
    // All spawn events configurable in the Details Panel — add entries here to register new event types
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director")
    TArray<FEventSpawnConfig> EventSpawnConfigs;

    // Weighted random selection from EventSpawnConfigs eligible at FinalIntensityScore.
    // Returns a default FEventSpawnConfig (EventName = NAME_None) if no config is eligible.
    UFUNCTION(BlueprintCallable, Category = "Director")
    FEventSpawnConfig SelectSpawnConfig(float FinalIntensityScore);

    // Returns true if Config has a valid EventName (not NAME_None).
    // Use this in Blueprint after SelectSpawnConfig to verify a meaningful result was returned.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Director")
    bool IsValidSpawnConfig(FEventSpawnConfig Config);
};
