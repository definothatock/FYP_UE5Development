// Copyright (C) Created by Ad, UST 2526 FYP team, code AR1.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DrawDebugHelpers.h"

#include "Character/NPC/SharedData/MemorySystem/Struct/FWorkingMemoryEntry.h"
#include "Character/NPC/SharedData/MemorySystem/System/MemSys_WorkingMemoryScoringLib.h"

#include "MemSys_WorkingMemoryArray.generated.h"


/*
 * ====================
 * Helper struct
 * ====================
 */

USTRUCT(BlueprintType)
struct FWorkingMemoryQueryResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query Result")
	bool IsFound = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query Result")
	FGuid EntryGuid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Query Result")
	FWorkingMemoryEntry FoundEntry; // Cannot Forward-Declare because compiler needs to know the size of the struct.
};

/*
 * ====================
 * Delegates
 * ====================
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryEntryAdded, const FWorkingMemoryEntry&, NewEntry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMemoryEntryRemoved, FGuid, EntryGuid, AActor*, Subject);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMemoryEntryRefreshed, const FWorkingMemoryEntry&, RefreshedEntry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMemoryArrayReset);

/*
 * ========================================
 * Actor component that maintains an array of Working Memory, mimicking part of Cortex's function. (or caching, in computer-ish)
 * This Array Maintains the "Truth". Requires to use in tandem with Evaluator ("thinking") and ScoringLib ("Opinion").
 * 
 * Small Array:
 * Entries order does not matter and order will be scrambled upon updates.
 *
 * Event-driven Elapsed Decay:
 * Does not decay overtime if nothing happened. Required periodic decay call.
 * 
 * Auto Mutation Decay:
 * Decay on all entry mutation / array refresh.
 * ========================================
 */
UCLASS(ClassGroup=(AIMemory), meta=(BlueprintSpawnableComponent))
class UST_FYP_API UMemSys_WorkingMemoryArray : public UActorComponent
{
	GENERATED_BODY()

	/*
	 * ==================== Array Configs ====================
	 * Set in Component setting panel. Forbid BP edits.
	 */

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Contents")
	TArray<FWorkingMemoryEntry> WorkingMemoryArray;
	// This means how many immanent Subject that self can handle. Should stay the same after construction. 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "2", ClampMax = "16"))
	int32 MaxEntries = 8;
	// Legacy tick decay. Kept for fallback/debug only.
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	// bool IsAutoTick = false;
	// the default refresh/init TTL. Currently, this is not directly used in InsertOrRefreshEntry(). Require manual feeding in BP.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "1.0"))
	float DefaultTTL = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "1.0"))
	float MaxTTL = 20.f;
	// Extra TTL to add when needed (not used as of 4/1/26)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "0.0"))
	float RefreshTTL = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config", meta = (ClampMin = "0.0"))
	float TTLDecayMultiplier = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Owner Info", meta = (ClampMin = "0.0"))
	FWorkingMemoryScoringProfile ScoringProfile;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Owner Info", meta = (ClampMin = "0.0"))
	FWorkingMemoryTopScoresFromEntries TopScores;
	
	// Should I store the PrimaryTarget in here or directly in Evaluator?
	
	/*
	 * ==================== System Setups ====================
	 */
		
public:
	UMemSys_WorkingMemoryArray();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	/*
	* ==================== Array Managers ====================
	*/

public:
	UFUNCTION(BlueprintCallable, Category = "Array Managers")
	void SetMaxArraySize(int32 InMaxEntries);

	UFUNCTION(BlueprintCallable, Category = "Array Managers")
	void ResetMemoryArray();

private:
	void EnforceMaxEntries();

	/*
	 * ==================== Entries Manager ====================
	 */

public:
	/* Different Creatures have different relationships and pre-conditions. Working Memory is not in-charged of that.
	 * External source or system should handle Those "judgment", preprocess the memory entry.
	 * Note: Should make another / support refresh by ID if I ever decided to improve this. */
	UFUNCTION(BlueprintCallable, Category = "Entries Manager")
	bool InsertOrRefreshEntry(const FWorkingMemoryEntry& IncomingEntry,FGuid& OutEntryGuid);

	// Direct Entry removal
	UFUNCTION(BlueprintCallable, Category = "Entries Manager")
	bool RemoveEntry(AActor* Subject);

	// Entrance for Event-driven decay. calls internal validator first. 
	UFUNCTION(BlueprintCallable, Category = "Entries Manager")
	void NotifyMemoryEvent();
	
	//UFUNCTION(BlueprintCallable, Category = "Entries Manager")
	//void ResetTTL();

private:

	// Decay using elapsed time since LastTTLUpdateTime (event-time based)
	void DecayEntriesOnCall();

	// Legacy Decay called in tick.
	/*
	 * Decay Normal entries, unless:
	 * Actively sensed (line of sight, kept hearing, etc) - stop decaying
	 * Invalidations (Subject gone) - Immediate TTL = 0 
	 */
	 // void DecayEntryOnTick(float DeltaTime);
	
	void RemoveExpired();

	//	Small internal validator + wrapper. Pass to Decay.
	void TryAutoDecayOnEvent();
	
	/*
	 * ==================== Evaluation ====================
	 */

	// Currently only updating Threat, for testing.
	UFUNCTION(BlueprintCallable, Category = "Evaluation | Considerations")
	bool UpdateScores();

	/*
	* ==================== Getters ====================
	* Some are Not UFUNCTION. Standard BP can receive array that is returned by value, but cannot handle reference return type.
	* Unless, we add UFUNCTION(BlueprintPure) to it. But that would cause unnecessary overhead.
	* Every time some downstream value needs the output of this function, BP re‑evaluates the node (no exc pin).
	* So we just leave it as a normal C++ function. After all, I don't expect this to be used in BP that often.
	* 
	* Also, mutable array is NOT multi-thread safe. but none of my concern rn cuz I will not make it.
	*/

public:
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FWorkingMemoryQueryResult FindEntryByActor(const AActor* Subject) const;
	FWorkingMemoryEntry* FindEntryByActor_Mutable(const AActor* Subject);

	UFUNCTION(BlueprintCallable, Category = "Getters")
	FWorkingMemoryQueryResult FindEntryByGuid(const FGuid& Guid) const;
	FWorkingMemoryEntry* FindEntryByGuid_Mutable(const FGuid& Guid);
	
	inline const TArray<FWorkingMemoryEntry>& GetMemoryArray() const { return WorkingMemoryArray; }

	/* I should prob make these into templates. But standard C++ template cannot expose to BP. find a way if I have extra time (frankly I dont)*/

	//
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FWorkingMemoryQueryResult  FindEntryByHighestThreat() const;
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FWorkingMemoryQueryResult FindTopThreatAndValue() const;
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FWorkingMemoryQueryResult FindBestSightedSubject() const;

	// Directly Check Tokens
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FWorkingMemoryQueryResult FindFirstAvailableLKL() const;
	// Directly Check Tokens
	UFUNCTION(BlueprintCallable, Category = "Getters")
	FWorkingMemoryQueryResult FindFirstAvailableLPL() const;

	UFUNCTION(BlueprintCallable, Category = "Getters")
	FWorkingMemoryQueryResult FindFirstAvailableHeard() const;
	

	/*
	 * ==================== Events ====================
	 */
		
public:
	// Delegates for external systems to react to memory changes.
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMemoryEntryAdded OnMemoryEntryAdded;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMemoryEntryRemoved OnMemoryEntryRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMemoryEntryRefreshed OnMemoryEntryRefreshed;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnMemoryArrayReset OnMemoryArrayReset;


	
	/*
	 *  ==================== Helpers ====================
	 */
	 
public:
	// Draw debug box for all entries in the array.
	UFUNCTION(BlueprintCallable, Category = "Helpers")
	void DebugDrawMemoryEntries(float Duration = 1.f,
		FColor SensedBoxColor = FColor(140, 150, 20), FColor LastKnownBoxColor = FColor(140, 69, 20),
		FVector BoxExtent = FVector(50.f, 50.f, 50.f)) const
	{
		if (!GetOwner()) { return; }

		for (const FWorkingMemoryEntry& Entry : WorkingMemoryArray)
		{
			if (Entry.IsSubjectValid())
			{
				// Draw at Actor location if actively sensed, else draw at last known location.
				FColor BoxColor;
				FVector Location;
				if (Entry.IsActivelySensed)
				{
					Location = Entry.Subject->GetActorLocation();
					BoxColor = SensedBoxColor;
				}
				else
				{
					Location = Entry.LastKnownLocation;
					BoxColor = LastKnownBoxColor;
				}
				
				DrawDebugBox(GetWorld(), Location, BoxExtent, BoxColor, false, Duration, 0, 1.f);
			}
		}
	}
	
	// quick check for the number of Entries / if empty
	UFUNCTION(BlueprintCallable, Category = "Helpers")
	int32 GetCurrentEntries()
	{
		return WorkingMemoryArray.Num();
	}

	//
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Helpers")
	bool IsHandleGuidValid(FWorkingMemoryHandle Handle)
	{
		if (!Handle.EntryGuid.IsValid()) { return false; }

		FWorkingMemoryEntry* Entry = FindEntryByGuid_Mutable(Handle.EntryGuid);
		if (!Entry) { return false; }
		
		// if (Handle.Subject.IsValid() && Entry->Subject != Handle.Subject)
		//	return false;

		// if (!Entry->Subject.IsValid())
		//	return false;

		return true;
	}

private:
	// Return true when Owner is from Server instance.
	inline bool ValidateAuth() const { return GetOwner() && GetOwner()->HasAuthority(); }
	

};
