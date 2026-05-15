// Copyright (C) Created by Ad, UST 2526 FYP team, code AR1.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "FWorkingMemoryDataStructs.h"

#include "FWorkingMemoryEntry.generated.h"


/*
 * ====================
 * Enums for Working Memory Entry
 * ====================
 */



/* keep but DONT use. Too much */
// For Relationship Classification
UENUM(BlueprintType)
enum class ERelationshipType : uint8
{
	Unknown,		// Not known (ambiguous)
	Prey,			// Main food/loot source
	Friendly,		// Known peers
	Neutral,		// Known, not much relation
	Hostile,		// Will attempt to start a fight
	Predator,		// Domination Written in DNA
	Other,			// Non Creature or Debug
};

/* REMOVED, too much
UENUM(BlueprintType)
enum class ENoiseType : uint8
{
	Unknown,
	PlayerVoice,
	CreatureVoice,
	Movement,
	Impact,
	Explosion,
	Other
};
*/

/* REMOVED, too much
UENUM(BlueprintType)
enum class ESubjectType : uint8
{
	Unknown,
	Player,
	OtherCreature,
	Prop,
	EnvironmentalHazard,
	Misc,
	Other,
};
*/

/* Removed: Attention Priority has moved to utility based. Allow dynamic (and a bit unpredicable?) behaviours. 
 * // For Focuses (TargetActor) Switch
UENUM(BlueprintType)
enum class EAttentionPriority : uint8
{
	Ignorable,      // Props, neutral
	Ambient,        // Noticed but not important
	Relevant,       // Worth tracking
	Urgent,         // Immediate attention needed
	Critical        // Cannot ignore (attacking self, blocking escape)
};
*/


/*
 * ========================================
 * Entry of Working Memory, Holds transient info about a subject actor.
 * This is to mimic part of cognitive system, temporarily holds dynamic info to support quick tactical decision.
 * 
 * Subject-info are retrieved from Subject,
 * Evaluation-data are judged by the creature (self) which is going to hold this Memory.
 * ========================================
 */
USTRUCT(BlueprintType)
struct FWorkingMemoryEntry
{
	GENERATED_BODY()
	
	FWorkingMemoryEntry() = default;
	
	/*
	 * ==================== Entry Info ====================
	 */

	// Global Unique ID (Guid) For Query
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Entry Info")
	FGuid EntryGuid;

	// Safeguard: 0 for Default TTL length in the specific ArrayCortex (only works for insertion, if entry existed entry will be killed).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entry info")
	float TTL = 60.f;

	/* Timestamp of the last time TTL was updated. Force Update at created/refreshed/Decay.
	* Not to confused with LastSensedTime! This is for *TTL* management! */
	UPROPERTY(BlueprintReadOnly, Category = "Entry info | Force Update")
	float LastTTLUpdateTime = -1.f;
	
	/*
	 * ==================== Subject Identity ====================
	 */
	
	/* Weakptr for auto null when actor is destroyed.
	// Maybe move this to private (or const) if I really concern about data safety? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject info")
	TWeakObjectPtr<AActor> Subject = nullptr;
	
	// Special Subject that is persistent in memory.
	// Only useful when paired with long-term memory (not implemented).
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject info")
	// int UniqueSubjectID = -1;

	/*
	 * ==================== Classification ====================
	 * (set on entry, rarely changes)
	 */

	// DONT USE. Too much.
	// Preset/Init by predefined mapping.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject info")
	ERelationshipType RelationshipEnum = ERelationshipType::Unknown;

	/* Universal threat level of the subject, reflect the subject's domination in the World. -1 for debug ignore.
	 * Does NOT reflect specific creature relations. For example, A is generally not aggressive, but hyper hostile to B.
	 * This non-transitive relation require per creature/species mapping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject info", meta = (ClampMin = "-1.0"))
	float UniversalThreatLevel = 0.f;
	
	// This is for Player only.
	// Only useful when paired with long-term memory (not implemented).
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Subject info")
	// float Reputation = -1.f;
	
	/*
	 * ==================== Perception Data ====================
	 * (frequently update)
	 */

	/* This is EXCLUSIVE for continuous senses (e.g. sight)! Do NOT use in discrete senses (most senses: hearing, touch, hurt, etc)!
	 * -> If the Subject is continuously sensed, Reset TTL in Decay() (quantized).
	 * Decay in tick is too expensive, error is acceptable given that: EvaluateEntries() timer is decent +  stim-driven Decay.
	 * Even tho all senses flag true at incoming, discrete senses do NOT flag back to false. Furthermore, they already triggers perception each updates (so counter intuitively, discrete pulses "continuously" refreshes). There is no need to "hold" the decay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool IsActivelySensed = false;
	
	// Determined in Evaluator, fed in by Perception. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool IsInLineOfSight = false;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool Heard = false;

	// flag if the location is checked already
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool TokenHeard = false;

	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	// FName NoiseType = "None";
	
	// Determined in Evaluator, fed in by Perception. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	FVector LastKnownLocation = FVector::ZeroVector;
		
	// flag if the location is checked already
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool TokenLKL = false;
	
	// Last record of the boolean WasPredicted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	FVector LastPredictedLocation = FVector::ZeroVector;
			
	// flag if the location is checked already
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool TokenLPL = false;
	
	// Last record of the boolean WasPredicted.
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	// float LastPredictedLocation_Age = -1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perception")
	FPerceptionSignals PerceptionSignals;

	// Issue: Major overlapping with LastEvalTime in DataStructs.h. Consider keep either one. Whenever sensed sth, Entry will definitely update and reevaluate. 
	/* The last time the Perception system pulsed this entry. Force Update at created/refreshed, or when IsActivelySensed = true (timed update, in dacay).
	 * Not to confused with LastTTLUpdateTime! This is for calculating *scores*! */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception | Force Update")
	float LastSensedTime = 0.f;
	
	/*
	 * ==================== Evaluation Metrics ====================
	 * (for evaluator's use)
	 */

	/* Was the Subject Identified / Self knows who triggered? (can directly use any Subject data?)
	 * A stimulus source could be not enough to identify the actual subject. (eg hearing)
	 * Unless certain creature can tell the Subject indirectly (eg player voice, distinctive environment noise) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
	bool IsIdentified = false;

	// Lost track to Subject? (can use fresh Subject data?)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
	bool WasPredicted = false;
	
	// How much damage has this Subject inflected? (Currently this does not decay)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
	float AccumulatedDmgToSelf = 0.f;
		
	// Fed in by Perception. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
	float DistanceToSelf = 0.f;

	// Scores per entry. Anchor: Should I make total score instead of per entry? 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evaluation")
	FWorkingMemorySubjectScores Opinions;
	
	/*
	 * ==================== Targeting History ====================
	 * helps with target stability,
	 * Actual Target pointers in Evaluator.
	 */

	// Accumulated time as First Target
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting History")
	float TimeAsFirstTarget = 0.f;

	// As this very Entry, Was the Subject the primary engagement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting History")
	bool WasTargeted = false;
	
	/*
	 * ==================== Internal helper functions ====================
	 */
	
	// Currently just a thin wrapper. might be useful if Subject gone private.
	 inline bool IsSubjectValid() const { return Subject.IsValid(); }
};





/*
 * ====================
 * Helper Struct
 * ====================
 */

/*
 * A small wrapper for the below Entry struct. (for compensating how stupid BP is and not letting me pass struct by ref)
 * This is for Focus/Targeting in Evaluator, without frequently accessing the MemArray (when array is not updating).
 */
USTRUCT(BlueprintType)
struct FWorkingMemoryHandle
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FGuid EntryGuid;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TWeakObjectPtr<AActor> Subject;

	// Determined in Evaluator, fed in by Perception. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool IsInLineOfSight = false;

	// Determined in Evaluator, fed in by Perception. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	FVector LastKnownLocation = FVector::ZeroVector;
		
	// flag if the location is checked already
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool TokenLKL = false;
	
	// Last record of the boolean WasPredicted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	FVector LastPredictedLocation = FVector::ZeroVector;
			
	// flag if the location is checked already
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	bool TokenLPL = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float LastSwitchTime;
};


/*
 * Note: this is REPLACED by PDA_CreatureUniData. Practically speaking, there is no difference (obj overhead tho), but I learned it, might as well try it.
 * For fetching Universal Subject data from that Subject, through interface.
 */
/*
USTRUCT(BlueprintType)

struct FCreatureUniData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Universal Data")
	ESubjectType SubjectCategoryEnum = ESubjectType::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Universal Data")
	FGameplayTag Species = FGameplayTag::EmptyTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Universal Data", meta = (ClampMin = "-1", ClampMax = "10"))
	float UniversalThreatLevel = 0.f;
};
*/
