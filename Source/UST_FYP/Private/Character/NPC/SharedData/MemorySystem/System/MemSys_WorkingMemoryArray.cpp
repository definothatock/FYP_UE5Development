// Copyright (C) Created by Ad, UST 2526 FYP team, code AR1.


#include "Character/NPC/SharedData/MemorySystem/System/MemSys_WorkingMemoryArray.h"

#include "Core/HealthSystem/HealthSystemInterface.h"


DEFINE_LOG_CATEGORY(WorkingMemory);


/*
 * ==================== System Setups ====================
 */


UMemSys_WorkingMemoryArray::UMemSys_WorkingMemoryArray()
{
	PrimaryComponentTick.bCanEverTick = true; // allow legacy tick if needed
	PrimaryComponentTick.bStartWithTickEnabled = false;
	bAutoActivate = true;
}


void UMemSys_WorkingMemoryArray::BeginPlay()
{
	Super::BeginPlay();
	// System Only runs on server instance, log on client.
	// Anchor: remove after development phase 
	if (!ValidateAuth())
	{
		UE_LOG(WorkingMemory, Warning, TEXT("WorkingMemoryArrayCortex is designed for server-side use. Owner: %s"), *GetOwner()->GetName());
	}

	/*
	SetComponentTickEnabled(IsAutoTick);
	if (IsAutoTick)
	{
		UE_LOG(WorkingMemory, Warning, TEXT("%s - WorkingMemory is using LEGACY tick decay."),
			*GetOwner()->GetName());
	}
	*/
}


void UMemSys_WorkingMemoryArray::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Only runs on server.
	// if (IsAutoTick && ValidateAuth()) { DecayEntryOnTick(DeltaTime); }
}


/*
 * ==================== Array Managers ====================
 */


void UMemSys_WorkingMemoryArray::SetMaxArraySize(int32 InMaxEntries)
{
	MaxEntries = InMaxEntries;
	EnforceMaxEntries();
}


// Get rid of the lowest ValueScore When forcing Max WorkingMemory Entries.
void UMemSys_WorkingMemoryArray::EnforceMaxEntries()
{
	// repeat until within limit
	while (WorkingMemoryArray.Num() >= MaxEntries)
	{
		UE_LOG(WorkingMemory, Warning, TEXT("%s - EnforceMaxEntries: Scaning Entries..."), *GetOwner()->GetName());
		int32 LowestIndex = -1;
		float LowestValue = BIG_NUMBER;
		// search through memory 
		for (int32 i = 0; i < WorkingMemoryArray.Num(); i++)
		{
			if (WorkingMemoryArray[i].Opinions.ValueScore <= LowestValue)
			{
				LowestIndex = i;
				LowestValue = WorkingMemoryArray[i].Opinions.ValueScore;
			}
		}
		// Remove the Smallest
		if (LowestIndex != -1)
		{
			UE_LOG(WorkingMemory, Warning, TEXT("%s - EnforceMaxEntries: Removing entry with lowest ValueScore - %s."), *GetOwner()->GetName(), WorkingMemoryArray[LowestIndex].IsSubjectValid() ? *WorkingMemoryArray[LowestIndex].Subject->GetName() : TEXT("Invalid Subject"));
			OnMemoryEntryRemoved.Broadcast( // broadcast before removal
				WorkingMemoryArray[LowestIndex].EntryGuid,
				WorkingMemoryArray[LowestIndex].Subject.Get());
			WorkingMemoryArray.RemoveAtSwap(LowestIndex);
		}
		else // leave if LowestIndex is weirdly not set
		{
			UE_LOG(WorkingMemory, Warning, TEXT("%s - EnforceMaxEntries Warning: LowestIndex not found when enforcing max entries."), *GetOwner()->GetName());
			break;
		} 
	}
}


void UMemSys_WorkingMemoryArray::ResetMemoryArray()
{
	if (!ValidateAuth())
	{ return; }

	// Broadcast removal for all entries.
	for (const FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		if (Entry.Subject.IsValid())
		{
			OnMemoryEntryRemoved.Broadcast(Entry.EntryGuid,Entry.Subject.Get());
		}
	}
	
	// .Reset instead of .Empty to retain allocated memory space.
	WorkingMemoryArray.Reset();

	OnMemoryArrayReset.Broadcast();

	UE_LOG(WorkingMemory, Warning, TEXT("%s - ResetMemoryArray: Working Memory Array has been reset."), *GetOwner()->GetName());
}


/*
 * ==================== Entries Manager ====================
 */


// Insert new entry or refresh existing entry by Subject Actor pointer. No calculation is done inside this function call.
// ISSUE: bad performance, but rn array is small so I left it be.
bool UMemSys_WorkingMemoryArray::InsertOrRefreshEntry(const FWorkingMemoryEntry& IncomingEntry,FGuid& OutEntryGuid)
{
	if (!ValidateAuth())
	{ return false; }

	TryAutoDecayOnEvent();

	if (!IncomingEntry.IsSubjectValid())
	{
		UE_LOG(WorkingMemory, Warning, TEXT("%s - InsertOrRefreshEntry: Attempted to insert entry with invalid Subject."), *GetOwner()->GetName());
		return false;
	}

	AActor* SubjectPtr = IncomingEntry.Subject.Get();
	float CurrentTime = GetWorld()->GetTimeSeconds();

	// ========== Refresh existing entry
	if (FWorkingMemoryEntry* ExistingEntry = FindEntryByActor_Mutable(SubjectPtr))
	{

		UE_LOG(WorkingMemory, Warning, TEXT("%s - InsertOrRefreshEntry: Found exiting entry: %s - %s."), *GetOwner()->GetName(), *SubjectPtr->GetName(), *ExistingEntry->EntryGuid.ToString());
		
		
		/* Subject Identity & Classification. Preservation. */
		const FGuid    PreservedGuid					=	ExistingEntry->EntryGuid;
		const float    PreservedUniversalThreatLevel    =	ExistingEntry->UniversalThreatLevel;
		// const PreservedRelation;

		/* Shallow copy of simple Data. */
		*ExistingEntry = IncomingEntry;
		
		/* Perception Data.
		 * accept Latest 
		ExistingEntry->IsActivelySensed = IncomingEntry.IsActivelySensed;
		ExistingEntry->IsInLineOfSight = IncomingEntry.IsInLineOfSight;
		ExistingEntry->Heard = IncomingEntry.Heard;
		ExistingEntry->TokenHeard = IncomingEntry.TokenHeard;
		// ExistingEntry->NoiseType = IncomingEntry.NoiseType;
		ExistingEntry->LastKnownLocation = IncomingEntry.LastKnownLocation;
		ExistingEntry->TokenLKL = IncomingEntry.TokenLKL;
		ExistingEntry->LastPredictedLocation = IncomingEntry.LastPredictedLocation;
		ExistingEntry->TokenLPL = IncomingEntry.TokenLPL;
		// ExistingEntry->LastPredictedLocation_Age = IncomingEntry.LastPredictedLocation_Age;
		ExistingEntry->PerceptionSignals = IncomingEntry.PerceptionSignals;
		/* Evaluation Metrics.
		 * accept Latest 
		ExistingEntry->IsIdentified = IncomingEntry.IsIdentified;
		ExistingEntry->WasPredicted = IncomingEntry.WasPredicted;
		// ExistingEntry->AttentionPriority = IncomingEntry.AttentionPriority;
		// if (IncomingEntry.LastMotivationToSubject.IsValid()) { ExistingEntry->LastMotivationToSubject = IncomingEntry.LastMotivationToSubject; }
		ExistingEntry->AccumulatedDmgToSelf = IncomingEntry.AccumulatedDmgToSelf;
		ExistingEntry->DistanceToSelf = IncomingEntry.DistanceToSelf;
		ExistingEntry->Opinions = IncomingEntry.Opinions;
		/* Targeting History.
		* accept Latest 
		ExistingEntry->TimeAsFirstTarget = IncomingEntry.TimeAsFirstTarget;
		ExistingEntry->WasTargeted = IncomingEntry.WasTargeted;

		/* Restore preserved fields */
		ExistingEntry->EntryGuid			=	PreservedGuid;
		ExistingEntry->UniversalThreatLevel =	PreservedUniversalThreatLevel;
		
		/* System-managed timestamps. Specific updates */
		float ClampedTTL					=	(IncomingEntry.TTL > KINDA_SMALL_NUMBER) ? IncomingEntry.TTL : DefaultTTL; // catch too small, set default 
		ExistingEntry->TTL					=	(ClampedTTL > MaxTTL) ? MaxTTL : ClampedTTL; // catch too big, set Max
		ExistingEntry->LastTTLUpdateTime	=	CurrentTime; // TTL updated, refresh
		ExistingEntry->LastSensedTime		=	CurrentTime; // directly get latest time

		
		UE_LOG(WorkingMemory, Warning, TEXT("%s - InsertOrRefreshEntry: Refreshed existing entry for Subject %s."), *GetOwner()->GetName(), *SubjectPtr->GetName());

		OnMemoryEntryRefreshed.Broadcast(*ExistingEntry);
		OutEntryGuid = ExistingEntry->EntryGuid; // BP return
		return true;
	}
	else // ========== Insert new entry
	{
		// Make room
		if (WorkingMemoryArray.Num() >= MaxEntries)
		{ 
			UE_LOG(WorkingMemory, Warning, TEXT("%s - InsertOrRefreshEntry: Working Memory full, enforcing max entries."), *GetOwner()->GetName());
			EnforceMaxEntries(); 
		}

		// Direct shallow copy incoming entry (should be fine as no pointer members)
		FWorkingMemoryEntry NewEntry = IncomingEntry;
		NewEntry.EntryGuid = FGuid::NewGuid();
		NewEntry.LastSensedTime = CurrentTime;
		NewEntry.LastTTLUpdateTime = CurrentTime;

		if (NewEntry.TTL <= KINDA_SMALL_NUMBER) { NewEntry.TTL = DefaultTTL; }

		WorkingMemoryArray.Add(NewEntry);

		UE_LOG(WorkingMemory, Warning, TEXT("%s - InsertOrRefreshEntry: Inserted new entry for Subject %s, Guid: %s"), *GetOwner()->GetName(), *SubjectPtr->GetName(), *NewEntry.EntryGuid.ToString());
		
		OnMemoryEntryAdded.Broadcast(NewEntry);
		OutEntryGuid = NewEntry.EntryGuid; // BP return
		return true;
	}
}


// Direct removal of entry by Subject Actor pointer. Return Successful or not.
bool UMemSys_WorkingMemoryArray::RemoveEntry(AActor* Subject)
{
	if (!ValidateAuth() || Subject == nullptr)
	{ return false; }

	TryAutoDecayOnEvent();
	
	// Remove all matching entries (supposed to be only one, but just in case).
	TArray<FGuid> RemovedGuids;
	const int32 RemovedCount = WorkingMemoryArray.RemoveAllSwap(
		[Subject, &RemovedGuids](const FWorkingMemoryEntry& Entry)
		{
			const bool bMatches = (Entry.Subject.Get() == Subject);
			if (bMatches)
			{
				RemovedGuids.Add(Entry.EntryGuid);
			}
			return bMatches;
		}
	);

	// Check removal
	if (RemovedCount > 0)
	{
		if (RemovedCount > 1)
		{
			UE_LOG(WorkingMemory, Warning, TEXT("%s - RemoveEntry: Removed %d duplicate entries for Subject %s"), *GetOwner()->GetName(), RemovedCount, *Subject->GetName());
		} else
		{
			UE_LOG(WorkingMemory, Warning, TEXT("%s - RemoveEntry: Removed entry for Subject %s"), *GetOwner()->GetName(), *Subject->GetName());
		}

		for (const FGuid& RemovedGuid : RemovedGuids)
		{
			OnMemoryEntryRemoved.Broadcast(RemovedGuid, Subject);
		}
		return true;
	}
	
	// No such Subject
	UE_LOG(WorkingMemory, Warning, TEXT("%s - RemoveEntry failed: Subject %s is not found"), *GetOwner()->GetName(), *Subject->GetName());
	return false;
}


void UMemSys_WorkingMemoryArray::NotifyMemoryEvent()
{
	TryAutoDecayOnEvent();
}


void UMemSys_WorkingMemoryArray::DecayEntriesOnCall()
{
	if (!ValidateAuth() || WorkingMemoryArray.Num() == 0 || !GetWorld())
	{ return; }

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	for (FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		// If Subject is invalid/destroyed, set TTL to 0 to mark for removal.
		if (!Entry.IsSubjectValid())
		{
			Entry.TTL = 0.f;
			Entry.LastTTLUpdateTime = CurrentTime;
			continue;
		}

		// If Subject being actively sensed, reset TTL and skip decay
		if (Entry.IsActivelySensed)
		{
			Entry.TTL = DefaultTTL;
			Entry.LastSensedTime = CurrentTime;
			Entry.LastTTLUpdateTime = CurrentTime;
			continue;
		}

		// event-time decay: subtract elapsed since last ttl update
		const float LastTime = (Entry.LastTTLUpdateTime >= 0.f) ? Entry.LastTTLUpdateTime : CurrentTime;
		const float Elapsed = FMath::Max(0.f, CurrentTime - LastTime);

		if (Elapsed > KINDA_SMALL_NUMBER)
		{
			Entry.TTL = FMath::Max(0.f, Entry.TTL - (Elapsed * TTLDecayMultiplier));
			Entry.LastTTLUpdateTime = CurrentTime;
		}
	}

	RemoveExpired();
}


// LEGACY Decay called in tick.
// Decay all entries' TTL by DeltaTime scaled by Decay Multiplier, Then call RemoveExpired().
/*
void UMemSys_WorkingMemoryArray::DecayEntryOnTick(float DeltaTime)
{
	if ( !ValidateAuth() || WorkingMemoryArray.Num() == 0 )
	{ return; }
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	for (FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		// If Subject is invalid/destroyed, set TTL to 0 to mark for removal.
		if (!Entry.IsSubjectValid())
		{
			Entry.TTL = 0.f;
			continue;
		}

		// If Subject being actively sensed, reset TTL and skip decay
		if (Entry.IsActivelySensed == true)
		{
			Entry.TTL = DefaultTTL;
			Entry.LastSensedTime = CurrentTime;
			Entry.LastTTLUpdateTime = CurrentTime;
			continue;
		}

		Entry.TTL = FMath::Max(0.f, Entry.TTL - (DeltaTime * TTLDecayMultiplier));
	}

	RemoveExpired();
}
*/


// Remove entries that are expired or whose subject is no longer valid.
void UMemSys_WorkingMemoryArray::RemoveExpired()
{
		struct FRemovedEntryInfo
		{
				FGuid Guid;
				TWeakObjectPtr<AActor> Subject;
		};
		TArray<FRemovedEntryInfo> RemovedEntries;

	
	// Call remove with predicate
	WorkingMemoryArray.RemoveAllSwap(
		[&RemovedEntries](const FWorkingMemoryEntry& Entry)
		{
			// If the pointer is null means Subject is already destroyed by engine.
			if (!Entry.IsSubjectValid())
			{
				// Subject is null here, so we can't add it to RemovedSubjects 
				// If that is the case, I think it is impossible to know what it was atp?
				RemovedEntries.Add({Entry.EntryGuid, nullptr}); 
				return true; 
			}
			
			// This is required if the subject is using HealthSystem. For example, player is 'killed',
			// but to keep the ragdoll and allow reviving, player's Character cannot be entirely removed.
			// Thus, we must go into the System and explicitly ask is it 'dead' gameplay wise.
			bool IsDead = false;
			if (Entry.Subject->Implements<UHealthSystemInterface>())
			{ IsDead = IHealthSystemInterface::Execute_IGetIsHealthDepleted(Entry.Subject.Get()); }  // some UE++ magic, TLDR just do this if I want to use interface that support both BP and C++

			// standard entry validity check
			const bool ShouldRemove = ( Entry.TTL <= KINDA_SMALL_NUMBER || IsDead);
			
			if (ShouldRemove)
			{
				RemovedEntries.Add({Entry.EntryGuid, Entry.Subject});
			}
			return ShouldRemove;
		}
	);
	// Broadcast and renumber 
	if (RemovedEntries.Num() > 0)
	{
		for (const FRemovedEntryInfo& Info : RemovedEntries)
		{
			AActor* const Subject = Info.Subject.Get();
			UE_LOG(WorkingMemory, Warning, TEXT(" %s - RemoveExpired: Removed expired entry for Subject %s."), *GetOwner()->GetName(), Subject ? *Subject->GetName() : TEXT("Invalid/Destroyed Subject"));

			OnMemoryEntryRemoved.Broadcast(Info.Guid, Subject);
		}
	}
}

// Event decay when NOT tick driven.
void UMemSys_WorkingMemoryArray::TryAutoDecayOnEvent()
{
	// if (!IsAutoTick)
	DecayEntriesOnCall();

}


/*
 * ==================== Evaluation ====================
 */


bool UMemSys_WorkingMemoryArray::UpdateScores()
{
	if (!ValidateAuth() || WorkingMemoryArray.Num() == 0)
	{ return false; }

	TryAutoDecayOnEvent();

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	for (FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		if (!Entry.IsSubjectValid()) { continue; }
		
		const float ComputedThreat = UMemSys_WorkingMemoryScoringLib::ComputeThreatScore(Entry, ScoringProfile, CurrentTime);
		Entry.Opinions.ThreatScore = ComputedThreat;
		Entry.Opinions.ThreatAndValueScore = ComputedThreat + Entry.Opinions.ValueScore;
		Entry.Opinions.LastEvalTime = CurrentTime;
		UE_LOG(WorkingMemory, Warning, TEXT("%s - Updated %s: ThreatScore - %f, ValueScore - %f, ThreatAndValue - %f. Last Eval: %f"), *GetOwner()->GetName(), *Entry.Subject->GetName(), ComputedThreat, Entry.Opinions.ValueScore, Entry.Opinions.ThreatAndValueScore, Entry.Opinions.LastEvalTime);
	}

	UE_LOG(WorkingMemory, Warning, TEXT("%s - UpdateThreatScores ran on %d entries."), *GetOwner()->GetName(), WorkingMemoryArray.Num());
	return true;
}


/*
 * ==================== Getters ====================
 */


 // Read only access to the entry.
FWorkingMemoryQueryResult UMemSys_WorkingMemoryArray::FindEntryByActor(const AActor* Subject) const
{
	FWorkingMemoryQueryResult QueryResult;
	if (WorkingMemoryArray.Num() == 0) { return QueryResult; }
	if (Subject == nullptr) { return QueryResult; }

	for (const FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		if (Entry.Subject.Get() == Subject)
		{
			QueryResult.IsFound = true;
			QueryResult.FoundEntry = Entry;
			QueryResult.EntryGuid = Entry.EntryGuid;
			return QueryResult;
		}
	}
	
	return QueryResult;
}


// Return mutable pointer to the entry for modification.
FWorkingMemoryEntry* UMemSys_WorkingMemoryArray::FindEntryByActor_Mutable(const AActor* Subject)
{
	if (WorkingMemoryArray.Num() == 0) { return nullptr; }
	if (Subject == nullptr) { return nullptr; }
	
	for (FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		if (Entry.Subject.Get() == Subject)
		{ return &Entry; }
	}
	
	return nullptr;
}


FWorkingMemoryQueryResult UMemSys_WorkingMemoryArray::FindEntryByGuid(const FGuid& Guid) const
{
	FWorkingMemoryQueryResult QueryResult;
	if (WorkingMemoryArray.Num() == 0) { return QueryResult; }
	if (!Guid.IsValid()) { return QueryResult; }

	for (const FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		if (Entry.EntryGuid == Guid)
		{
			QueryResult.IsFound = true;
			QueryResult.FoundEntry = Entry;
			QueryResult.EntryGuid = Entry.EntryGuid;
			return QueryResult;
		}
	}
	
	return QueryResult;
}


FWorkingMemoryEntry* UMemSys_WorkingMemoryArray::FindEntryByGuid_Mutable(const FGuid& Guid)
{
	if (WorkingMemoryArray.Num() == 0) { return nullptr; }
	if (!Guid.IsValid()) { return nullptr; }

	for (FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		if (Entry.EntryGuid == Guid)
		{ return &Entry; }
	}
	
	return nullptr;
}


FWorkingMemoryQueryResult UMemSys_WorkingMemoryArray::FindEntryByHighestThreat() const
{
	FWorkingMemoryQueryResult QueryResult;
	if (WorkingMemoryArray.Num() == 0) { return QueryResult; }

	int32 HighestIndex = -1;
	float HighestThreat = -255.0;
	for (int32 i = 0; i < WorkingMemoryArray.Num(); i++)
	{
		if (WorkingMemoryArray[i].Opinions.ThreatScore > HighestThreat)
		{
			HighestIndex = i;
			HighestThreat = WorkingMemoryArray[i].Opinions.ThreatScore;
		}
	}

	if (HighestIndex == -1) { return QueryResult; }

	// edge case: all ThreatScore are 0 - issue ignored. dawg just pick sth.

	QueryResult.IsFound = true;
	QueryResult.FoundEntry = WorkingMemoryArray[HighestIndex];
	QueryResult.EntryGuid = WorkingMemoryArray[HighestIndex].EntryGuid;
	return QueryResult;
}


FWorkingMemoryQueryResult UMemSys_WorkingMemoryArray::FindTopThreatAndValue() const
{
	FWorkingMemoryQueryResult QueryResult;
	if (WorkingMemoryArray.Num() == 0) { return QueryResult; }

	int32 HighestIndex = -1;
	float HighestScore = -255.0;
	for (int32 i = 0; i < WorkingMemoryArray.Num(); i++)
	{
		if (WorkingMemoryArray[i].Opinions.ThreatAndValueScore > HighestScore)
		{
			HighestIndex = i;
			HighestScore = WorkingMemoryArray[i].Opinions.ThreatAndValueScore;
		}
	}

	if (HighestIndex == -1) { return QueryResult; }

	// edge case: all ThreatScore are 0 - issue ignored. dawg just pick sth.

	QueryResult.IsFound = true;
	QueryResult.FoundEntry = WorkingMemoryArray[HighestIndex];
	QueryResult.EntryGuid = WorkingMemoryArray[HighestIndex].EntryGuid;
	return QueryResult;
}


FWorkingMemoryQueryResult UMemSys_WorkingMemoryArray::FindBestSightedSubject() const
{
	FWorkingMemoryQueryResult QueryResult;
	if (WorkingMemoryArray.Num() == 0) { return QueryResult; }

	int32 HighestIndex = -1;
	float HighestScore = -255.0;
	for (int32 i = 0; i < WorkingMemoryArray.Num(); i++)
	{	// For all visioned: choose most juicy
		if (WorkingMemoryArray[i].IsInLineOfSight == true && WorkingMemoryArray[i].Opinions.ThreatAndValueScore > HighestScore)
		{
			HighestIndex = i;
			HighestScore = WorkingMemoryArray[i].Opinions.ThreatAndValueScore;
		}
	}

	if (HighestIndex == -1) { return QueryResult; }
	
	QueryResult.IsFound = true;
	QueryResult.FoundEntry = WorkingMemoryArray[HighestIndex];
	QueryResult.EntryGuid = WorkingMemoryArray[HighestIndex].EntryGuid;
	return QueryResult;
}


FWorkingMemoryQueryResult UMemSys_WorkingMemoryArray::FindFirstAvailableLKL() const
{
	FWorkingMemoryQueryResult QueryResult;
	if (WorkingMemoryArray.Num() == 0) { return QueryResult; }

	for (const FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		if (Entry.TokenLKL == true)
		{
			QueryResult.IsFound = true;
			QueryResult.FoundEntry = Entry;
			QueryResult.EntryGuid = Entry.EntryGuid;
			return QueryResult;
		}
	}
	
	return QueryResult;
}


FWorkingMemoryQueryResult UMemSys_WorkingMemoryArray::FindFirstAvailableLPL() const
{
	FWorkingMemoryQueryResult QueryResult;
	if (WorkingMemoryArray.Num() == 0) { return QueryResult; }

	for (const FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		if (Entry.TokenLPL == true)
		{
			QueryResult.IsFound = true;
			QueryResult.FoundEntry = Entry;
			QueryResult.EntryGuid = Entry.EntryGuid;
			return QueryResult;
		}
	}
	
	return QueryResult;
}


FWorkingMemoryQueryResult UMemSys_WorkingMemoryArray::FindFirstAvailableHeard() const
{
	FWorkingMemoryQueryResult QueryResult;
	if (WorkingMemoryArray.Num() == 0) { return QueryResult; }

	for (const FWorkingMemoryEntry& Entry : WorkingMemoryArray)
	{
		if (Entry.TokenHeard == true)
		{
			QueryResult.IsFound = true;
			QueryResult.FoundEntry = Entry;
			QueryResult.EntryGuid = Entry.EntryGuid;
			return QueryResult;
		}
	}
	
	return QueryResult;
}


/*
 *  ==================== Helpers ====================
 */



