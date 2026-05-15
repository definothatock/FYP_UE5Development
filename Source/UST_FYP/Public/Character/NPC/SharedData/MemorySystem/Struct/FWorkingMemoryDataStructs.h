// Copyright (C) Created by Ad, UST 2526 FYP team, code AR1.

#pragma once

#include "CoreMinimal.h"
#include "FWorkingMemoryDataStructs.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(WorkingMemory, Warning, All);

/* NOTICE!
 * Due to time and man power constrain, Direction of the AI has changed: Player focus. AI don't are about each other, they eliminate player only.
 */


/*
 * ====================
 * Interface structs
 * ====================
 */


/*
 * Interface: for quick AI sense tweaking.
 * Meant to be stored in Controller or any other actual owner of the working memories, not per Work Mem entry.
 * For calculating final Scores.
 */
USTRUCT(BlueprintType)
struct FWorkingMemoryScoringProfile
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Scoring Profile")
	float RecencyHalfLife = 10.0f;
	// To limit jittering. How much more that the current Subject to trigger a switch
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Scoring Profile")
	// float SwitchMargin = 3.0f;
	// Also to limit jittering (or lower it to make self do some funny emote)
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Scoring Profile")
	// float SwitchCooldownSeconds = 1.0f;

	/* Perception Sense Bias. */


	// How "strong" self feels "pain".
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Scoring Profile | Sense Bias")
	float DamageStim_Amplifier = 5.0f;
};


/*
 * ====================
 * Internal Structs
 * ====================
 */


/*
 * Perception Signals per entry (how valid/important is the stimulus?), fetched from raw sense data.
 * All Normalized to 0~1 (Clamped).
 */
USTRUCT(BlueprintType)
struct FPerceptionSignals
{
	GENERATED_BODY()
	
	// 1 = clear and confirmed, accumulated LOS. Issue: unreal's vision is definite. Need to make a vision cure elsewhere.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  meta = (ClampMin = "0.0", ClampMax = "1.0"), Category="Working Memory | Perception Scales | Observation")
	float VisionStimFactor = 0.f;
	// Clamp(audio strength). Already involved with relative distance in Unreal (End-Of-BucketList: for shocking with "huge audio spike" like explosion, do it elsewhere.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  meta = (ClampMin = "0.0", ClampMax = "1.0"), Category="Working Memory | Perception Scales | Observation")
	float AudioStimFactor = 0.f;
	// AccumulatedDmg / MaxHP, The most direct way to tell if the Subject is a threat.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  meta = (ClampMin = "0.0", ClampMax = "1.0"), Category="Working Memory | Perception Scales | Imminent")
	float DamageStimFactor = 0.f;
	// 1 - Clamp(Distance / SafeRange), How close is the Subject to self.
	// Be mindful with double distance scaling. some sense already scaled with distance.
	// UPROPERTY(EditAnywhere, BlueprintReadWrite,  meta = (ClampMin = "0.0", ClampMax = "1.0"),Category="Working Memory | Perception Scales")
	// float DistanceFactor = 0.f;
};


/*
 * Final Scores per entry, calculated form Subject Data.
 */
USTRUCT(BlueprintType)
struct FWorkingMemorySubjectScores
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Derived Scores")
	float ThreatScore = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Derived Scores")
	float ValueScore = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Derived Scores")
	float ThreatAndValueScore = 1.f;
	
	// Issue: Major overlapping with LastSensedTime in Entry.h. Consider keep either one. Whenever sensed sth, Entry will definitely update and reevaluate. 
	// for calculating Lazy HalfLife.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Derived Scores")
	float LastEvalTime = 0.f;
};

/*
 * Non duplicated Scores own by The MemArray owner (self).
 * For self evaluation.
 */
USTRUCT(BlueprintType)
struct FWorkingMemoryTopScoresFromEntries
{
	GENERATED_BODY()


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Working Memory | Global Scores")
	float TopThreatAndValue = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Working Memory | Global Scores")
	FGuid TopThreatAndValueGuid;

	/*
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Global Scores")
	float TotalThreat = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Working Memory | Global Scores")
	float TopThreat = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Working Memory | Global Scores")
	FGuid TopThreatGuid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Working Memory | Global Scores")
	float TopValue = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Working Memory | Global Scores")
	FGuid TopValueGuid;
	*/
};	