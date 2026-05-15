// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Minigame_Slider.generated.h"

class UTextRenderComponent;
class UShapeComponent;
class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct FRoundConfig
{
    GENERATED_BODY()

    // Number of shrooms to pop simultaneously each spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ShroomsPerSpawn = 1;

    // Number of shrooms to stomp before advancing to next round
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ThresholdToAdvance = 3;

    // How long (seconds) a shroom stays up before retracting
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PopDuration = 3.0f;

    // Interval (seconds) between each spawn wave
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnInterval = 2.0f;
};

UCLASS()
class UST_FYP_API AMinigame_Slider : public AActor
{
    GENERATED_BODY()

public:
    AMinigame_Slider();

    virtual void BeginPlay() override;

    // Round configuration array; edit in Details Panel to define all rounds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    TArray<FRoundConfig> RoundConfigs;

    // Called by BP_DefaultPlayer's Server RPC when player stomps a shroom
    UFUNCTION(BlueprintCallable, Category = "Shroom")
    void Server_HitShroom(int32 ShroomIndex);

    // Initialize game state and start the first round; called by BeginPlay on Authority
    UFUNCTION(BlueprintCallable, Category = "Shroom")
    void StartGame();

    // Called by C++ when a shroom pops up; override in Blueprint to handle visuals
    UFUNCTION(BlueprintImplementableEvent, Category = "Shroom")
    void CallOnShroomAppear(int32 ShroomIndex);

    // Called by C++ when a shroom retracts (timer expired or stomped); override in Blueprint to handle visuals
    UFUNCTION(BlueprintImplementableEvent, Category = "Shroom")
    void CallOnShroomRetract(int32 ShroomIndex);

    // Called by C++ when score changes; override in Blueprint to update UI
    UFUNCTION(BlueprintImplementableEvent, Category = "Shroom")
    void CallOnScoreUpdated(int32 NewRound, int32 NewScore);

    // Called by C++ when round advances; override in Blueprint to play effects
    UFUNCTION(BlueprintImplementableEvent, Category = "Shroom")
    void CallOnRoundAdvanced(int32 NewRound);

    // Called by C++ when all rounds are completed; override in Blueprint to show win screen
    UFUNCTION(BlueprintImplementableEvent, Category = "Shroom")
    void CallOnGameSolved();
    // Difficulty config arrays; each represents one round's settings
    // Level 2 uses Level1 + Level2, Level 3 uses all three
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    TArray<FRoundConfig> Difficulty1Configs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    TArray<FRoundConfig> Difficulty2Configs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
    TArray<FRoundConfig> Difficulty3Configs;

    // Call before StartGame(); builds RoundConfigs from difficulty arrays
    UFUNCTION(BlueprintCallable, Category = "Shroom")
    void SetDifficulty(int32 Level);

    // Resets all game state; does not touch RoundConfigs
    UFUNCTION(BlueprintCallable, Category = "Shroom")
    void ResetGame();

    // Called by C++ when game is won; Blueprint handles reward spawn
    UFUNCTION(BlueprintImplementableEvent, Category = "Shroom")
    void CallOnRewardSpawn();

    // Number of missed shrooms allowed before game is lost
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
    int32 MissLimit = 3;

    // Called by C++ when player misses too many shrooms
    UFUNCTION(BlueprintImplementableEvent, Category = "Shroom")
    void CallOnGameLost();

    // Sign Component References - assigned from Blueprint BeginPlay
    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UTextRenderComponent* SignText_Main;

    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UTextRenderComponent* SignText_Diff;

    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UTextRenderComponent* SignText_1;

    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UTextRenderComponent* SignText_2;

    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UTextRenderComponent* SignText_3;

    // Round and Score display text renders - hidden until first shroom is stomped
    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UTextRenderComponent* SignText_Round;

    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UTextRenderComponent* SignText_Score;

    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UShapeComponent* SignBox_Play;

    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UShapeComponent* SignBox_1;

    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UShapeComponent* SignBox_2;

    UPROPERTY(BlueprintReadWrite, Category = "Sign")
    UShapeComponent* SignBox_3;

    // 0=Idle, 1=SelectDifficulty, 2=Playing, 3=Win, 4=Lost
    UFUNCTION(BlueprintCallable, Category = "Sign")
    void UpdateSignDisplay(int32 State);

    UFUNCTION(NetMulticast, Reliable, Category = "Sign")
    void Multicast_UpdateSignDisplay(int32 State);

    UFUNCTION(NetMulticast, Reliable, Category = "Sign")
    void Multicast_UpdateRoundScore(int32 NewRound, int32 NewScore);

    // Called from BP_DefaultPlayer when player clicks on the sign
    // Internally checks which box was hit and runs the correct logic
    UFUNCTION(BlueprintCallable, Category = "Sign")
    void HandleSignClick(UPrimitiveComponent* HitComponent);

private:
    // Start the given round: reset spawn timer and notify Blueprint
    void StartRound(int32 RoundIndex);

    // Spawn a wave of shrooms based on current round config
    void PopShrooms();

    // Called when a shroom's retract timer expires
    void RetractShroom(int32 ShroomIndex);

    // Advance to the next round; trigger win if all rounds done
    void AdvanceRound();

    // Start a retract countdown for the shroom at ShroomIndex
    void SetShroomRetractTimer(int32 ShroomIndex, float Delay);

    // Cancel the retract timer for the shroom at ShroomIndex
    void ClearShroomRetractTimer(int32 ShroomIndex);

    // Cancel all 16 retract timers
    void ClearAllShroomTimers();

    // Active state per shroom slot (indices 0-15); true = currently popped up
    TArray<bool> ActiveShrooms;

    // One FTimerHandle per shroom slot (indices 0-15)
    TArray<FTimerHandle> ShroomTimerHandles;

    // Timer handle for the periodic spawn wave
    FTimerHandle SpawnTimerHandle;

    int32 CurrentRound = 0;
    int32 CurrentRoundScore = 0;
    bool bIsSolved = false;

    // Tracks how many shrooms have been missed this game
    int32 MissCount = 0;

    // True once a difficulty is selected and game has started; blocks further sign clicks
    bool bIsPlaying = false;
};