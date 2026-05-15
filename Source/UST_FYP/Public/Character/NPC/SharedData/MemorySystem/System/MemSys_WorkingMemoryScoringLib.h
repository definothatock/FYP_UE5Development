// Copyright (C) Created by Ad, UST 2526 FYP team, code AR1.


#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Character/NPC/SharedData/MemorySystem/Struct/FWorkingMemoryEntry.h"

#include "MemSys_WorkingMemoryScoringLib.generated.h"


/*
 * ====================
 * Helper struct
 * ====================
 */


/*
 * ========================================
 * This library is for WorkingMemoryEntry Evaluation, assume these functions are directly in Evaluator.
 * ========================================
 */
UCLASS()
class UST_FYP_API UMemSys_WorkingMemoryScoringLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	
public:

	/*
	 * ==================== Opinions (Per entry) Scoring ====================
	 */
	
	UFUNCTION(BlueprintCallable, Category = "WorkingMemory | ScoringLib")
	static float ComputeThreatScore(const FWorkingMemoryEntry& Entry, const FWorkingMemoryScoringProfile& Profile, float CurrentTime);


	/*
	 * ==================== Helpers ====================
	 */
	
	// hard coded rn for testing. later should change to mappings.
	// UFUNCTION(BlueprintCallable, Category = "WorkingMemory | ScoringLib")
	// static float GetRelationshipMultiplier(const ERelationshipType Relationship);
	
};
