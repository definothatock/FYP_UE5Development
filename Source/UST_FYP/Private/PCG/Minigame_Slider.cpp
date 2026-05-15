// Fill out your copyright notice in the Description page of Project Settings.

#include "PCG/Minigame_Slider.h"
#include "TimerManager.h"
#include "Components/TextRenderComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/PrimitiveComponent.h"

AMinigame_Slider::AMinigame_Slider()
{
    bReplicates = true;
}

void AMinigame_Slider::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority()) return;

    // Initialize shroom slots
    ActiveShrooms.Init(false, 16);
    ShroomTimerHandles.Init(FTimerHandle(), 16);

    // StartGame removed - called after player selects difficulty via HandleSignClick
}

void AMinigame_Slider::StartGame()
{
    CurrentRound = 0;
    CurrentRoundScore = 0;
    bIsSolved = false;

    if (RoundConfigs.Num() == 0) return;

    StartRound(0);
}

void AMinigame_Slider::StartRound(int32 RoundIndex)
{
    GetWorldTimerManager().ClearTimer(SpawnTimerHandle);

    GetWorldTimerManager().SetTimer(
        SpawnTimerHandle,
        this,
        &AMinigame_Slider::PopShrooms,
        RoundConfigs[RoundIndex].SpawnInterval,
        true // bLoopupdat
    );

    CallOnRoundAdvanced(RoundIndex);
}

void AMinigame_Slider::PopShrooms()
{
    if (!RoundConfigs.IsValidIndex(CurrentRound)) return;

    const FRoundConfig& Config = RoundConfigs[CurrentRound];

    UE_LOG(LogTemp, Warning, TEXT("PopShrooms called: CurrentRound=%d, ShroomsPerSpawn=%d, ActiveCount=%d"), CurrentRound, Config.ShroomsPerSpawn, ActiveShrooms.Num());

    // Collect all available (not active) shroom indices
    TArray<int32> AvailableHoles;
    for (int32 i = 0; i < ActiveShrooms.Num(); i++)
    {
        if (!ActiveShrooms[i])
        {
            AvailableHoles.Add(i);
        }
    }

    // Shuffle and pick up to ShroomsPerSpawn indices
    int32 SpawnCount = FMath::Min(Config.ShroomsPerSpawn, AvailableHoles.Num());
    for (int32 i = AvailableHoles.Num() - 1; i > 0; i--)
    {
        int32 j = FMath::RandRange(0, i);
        AvailableHoles.Swap(i, j);
    }

    for (int32 k = 0; k < SpawnCount; k++)
    {
        int32 Index = AvailableHoles[k];
        UE_LOG(LogTemp, Warning, TEXT("PopShrooms: Spawning shroom at index %d, ActiveShrooms count: %d, AvailableHoles count: %d"), Index, ActiveShrooms.Num(), AvailableHoles.Num());
        ActiveShrooms[Index] = true;
        CallOnShroomAppear(Index);
        SetShroomRetractTimer(Index, Config.PopDuration);
    }
}

void AMinigame_Slider::RetractShroom(int32 ShroomIndex)
{
    if (!ActiveShrooms.IsValidIndex(ShroomIndex)) return;
    if (!ActiveShrooms[ShroomIndex]) return;

    ActiveShrooms[ShroomIndex] = false;
    CallOnShroomRetract(ShroomIndex);

    // Count as a miss - shroom retracted without being stomped
    MissCount++;
    if (MissCount >= MissLimit)
    {
        bIsSolved = true;
        ClearAllShroomTimers();
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
        UpdateSignDisplay(4);
        CallOnGameLost();
    }
}

void AMinigame_Slider::Server_HitShroom(int32 ShroomIndex)
{
    if (bIsSolved) return;
    if (!ActiveShrooms.IsValidIndex(ShroomIndex)) return;
    if (!ActiveShrooms[ShroomIndex]) return; // Shroom not active

    ClearShroomRetractTimer(ShroomIndex);
    ActiveShrooms[ShroomIndex] = false;
    CallOnShroomRetract(ShroomIndex);

    CurrentRoundScore++;
    CallOnScoreUpdated(CurrentRound, CurrentRoundScore);
    Multicast_UpdateRoundScore(CurrentRound, CurrentRoundScore);

    if (CurrentRoundScore >= RoundConfigs[CurrentRound].ThresholdToAdvance)
    {
        AdvanceRound();
    }
}

void AMinigame_Slider::AdvanceRound()
{
    CurrentRound++;
    CurrentRoundScore = 0;

    if (CurrentRound >= RoundConfigs.Num())
    {
        bIsSolved = true;
        ClearAllShroomTimers();
        GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
        UpdateSignDisplay(3);
        CallOnGameSolved();
    }
    else
    {
        StartRound(CurrentRound);
    }
}

void AMinigame_Slider::SetShroomRetractTimer(int32 ShroomIndex, float Delay)
{
    if (ShroomIndex < 0 || ShroomIndex > 15) return;

    GetWorldTimerManager().ClearTimer(ShroomTimerHandles[ShroomIndex]);

    GetWorldTimerManager().SetTimer(
        ShroomTimerHandles[ShroomIndex],
        [this, ShroomIndex]()
        {
            if (IsValid(this))
            {
                RetractShroom(ShroomIndex);
            }
        },
        Delay,
        false // bLoop
    );
}

void AMinigame_Slider::ClearShroomRetractTimer(int32 ShroomIndex)
{
    if (ShroomIndex < 0 || ShroomIndex > 15) return;

    GetWorldTimerManager().ClearTimer(ShroomTimerHandles[ShroomIndex]);
}

void AMinigame_Slider::ClearAllShroomTimers()
{
    for (FTimerHandle& Handle : ShroomTimerHandles)
    {
        GetWorldTimerManager().ClearTimer(Handle);
    }
}

void AMinigame_Slider::SetDifficulty(int32 Level)
{
    // Build RoundConfigs by appending difficulty layers
    RoundConfigs.Empty();
    if (Level >= 1) RoundConfigs.Append(Difficulty1Configs);
    if (Level >= 2) RoundConfigs.Append(Difficulty2Configs);
    if (Level >= 3) RoundConfigs.Append(Difficulty3Configs);
}

void AMinigame_Slider::ResetGame()
{
    // Reset runtime state only; RoundConfigs preserved until next SetDifficulty call
    bIsSolved = false;
    bIsPlaying = false;
    CurrentRound = 0;
    CurrentRoundScore = 0;
    MissCount = 0;
    ClearAllShroomTimers();
    GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
    ActiveShrooms.Init(false, 16);
}

void AMinigame_Slider::UpdateSignDisplay(int32 State)
{
    UE_LOG(LogTemp, Warning, TEXT("UpdateSignDisplay called State: %d"), State);
    if (HasAuthority())
    {
        // Server broadcasts to all clients
        Multicast_UpdateSignDisplay(State);
    }
    else
    {
        // Client executes locally (e.g. during BeginPlay initialization)
        Multicast_UpdateSignDisplay_Implementation(State);
    }
}

void AMinigame_Slider::Multicast_UpdateSignDisplay_Implementation(int32 State)
{
    // Helper lambda to safely set text render visibility
    auto SetTRVisible = [](UTextRenderComponent* Comp, bool bVisible)
    {
        if (Comp) Comp->SetVisibility(bVisible);
    };

    // Helper lambda to safely set box collision visibility
    auto SetBoxVisible = [](UShapeComponent* Comp, bool bVisible)
    {
        if (Comp) Comp->SetVisibility(bVisible);
    };

    // Helper lambda to safely set text
    auto SetTRText = [](UTextRenderComponent* Comp, FString Text)
    {
        if (Comp) Comp->SetText(FText::FromString(Text));
    };

    switch (State)
    {
        case 0: // Idle
            SetTRText(SignText_Main, "Play?");
            SetTRVisible(SignText_Main, true);
            SetTRVisible(SignText_Diff, false);
            SetTRVisible(SignText_1, false);
            SetTRVisible(SignText_2, false);
            SetTRVisible(SignText_3, false);
            SetTRVisible(SignText_Round, false);
            SetTRVisible(SignText_Score, false);
            SetBoxVisible(SignBox_Play, true);
            SetBoxVisible(SignBox_1, false);
            SetBoxVisible(SignBox_2, false);
            SetBoxVisible(SignBox_3, false);
            break;

        case 1: // SelectDifficulty
            SetTRText(SignText_Diff, "Difficulty:");
            SetTRText(SignText_1, "1");
            SetTRText(SignText_2, "2");
            SetTRText(SignText_3, "3");
            SetTRVisible(SignText_Main, false);
            SetTRVisible(SignText_Diff, true);
            SetTRVisible(SignText_1, true);
            SetTRVisible(SignText_2, true);
            SetTRVisible(SignText_3, true);
            SetTRVisible(SignText_Round, false);
            SetTRVisible(SignText_Score, false);
            SetBoxVisible(SignBox_Play, false);
            SetBoxVisible(SignBox_1, true);
            SetBoxVisible(SignBox_2, true);
            SetBoxVisible(SignBox_3, true);
            break;

        case 2: // Playing
            SetTRVisible(SignText_Main, false);
            SetTRVisible(SignText_Diff, false);
            SetTRVisible(SignText_1, false);
            SetTRVisible(SignText_2, false);
            SetTRVisible(SignText_3, false);
            SetTRVisible(SignText_Round, false);
            SetTRVisible(SignText_Score, false);
            SetBoxVisible(SignBox_Play, false);
            SetBoxVisible(SignBox_1, false);
            SetBoxVisible(SignBox_2, false);
            SetBoxVisible(SignBox_3, false);
            break;

        case 3: // Win
            SetTRText(SignText_Main, "WIN!");
            SetTRVisible(SignText_Main, true);
            SetTRVisible(SignText_Diff, false);
            SetTRVisible(SignText_1, false);
            SetTRVisible(SignText_2, false);
            SetTRVisible(SignText_3, false);
            SetTRVisible(SignText_Round, false);
            SetTRVisible(SignText_Score, false);
            SetBoxVisible(SignBox_Play, false);
            SetBoxVisible(SignBox_1, false);
            SetBoxVisible(SignBox_2, false);
            SetBoxVisible(SignBox_3, false);
            break;

        case 4: // Lost
            SetTRText(SignText_Main, "LOST");
            SetTRVisible(SignText_Main, true);
            SetTRVisible(SignText_Diff, false);
            SetTRVisible(SignText_1, false);
            SetTRVisible(SignText_2, false);
            SetTRVisible(SignText_3, false);
            SetTRVisible(SignText_Round, false);
            SetTRVisible(SignText_Score, false);
            SetBoxVisible(SignBox_Play, false);
            SetBoxVisible(SignBox_1, false);
            SetBoxVisible(SignBox_2, false);
            SetBoxVisible(SignBox_3, false);
            break;
    }
}

void AMinigame_Slider::Multicast_UpdateRoundScore_Implementation(int32 NewRound, int32 NewScore)
{
    // Show round and score text; NewRound + 1 because C++ index is 0-based
    if (SignText_Round)
    {
        SignText_Round->SetVisibility(true);
        SignText_Round->SetText(FText::FromString(FString::Printf(TEXT("Round: %d"), NewRound + 1)));
    }
    if (SignText_Score)
    {
        SignText_Score->SetVisibility(true);
        SignText_Score->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), NewScore)));
    }
}

void AMinigame_Slider::HandleSignClick(UPrimitiveComponent* HitComponent)
{
    if (!HasAuthority()) return;
    if (!HitComponent) return;
    if (bIsPlaying) return; // Block all clicks once game started

    if (HitComponent == SignBox_Play)
    {
        UpdateSignDisplay(1);
    }
    else if (HitComponent == SignBox_1)
    {
        bIsPlaying = true;
        SetDifficulty(1);
        StartGame();
        UpdateSignDisplay(2);
    }
    else if (HitComponent == SignBox_2)
    {
        bIsPlaying = true;
        SetDifficulty(2);
        StartGame();
        UpdateSignDisplay(2);
    }
    else if (HitComponent == SignBox_3)
    {
        bIsPlaying = true;
        SetDifficulty(3);
        StartGame();
        UpdateSignDisplay(2);
    }
}