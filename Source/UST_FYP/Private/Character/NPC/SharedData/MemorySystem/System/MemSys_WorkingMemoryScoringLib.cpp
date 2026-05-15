// Copyright (C) Created by Ad, UST 2526 FYP team, code AR1.

#include "Character/NPC/SharedData/MemorySystem/System/MemSys_WorkingMemoryScoringLib.h"




/*
 * ==================== Opinions (Per entry) Scoring ====================
 */


float UMemSys_WorkingMemoryScoringLib::ComputeThreatScore(const FWorkingMemoryEntry& Entry, const FWorkingMemoryScoringProfile& Profile, float CurrentTime)
{
	float Recency_Suppression = FMath::Exp2(-(CurrentTime - Entry.LastSensedTime) / Profile.RecencyHalfLife);

	float Known_Threat = 0.0f;
	
	// Self knows what this is?
	if (Entry.IsIdentified == true)
	{
		Known_Threat = Entry.UniversalThreatLevel;
	}
	
	// imminent threat. Absolute, ignores Recency. The "What".
	float Imminent_Threat = (Entry.PerceptionSignals.DamageStimFactor * Profile.DamageStim_Amplifier);

	// Potential_Threat already scaled with Recency_Suppression (Certainty).
	float FinalThreat = Imminent_Threat + (Known_Threat * Recency_Suppression);
	return FinalThreat;
}

/*
 * ==================== Helpers ====================
 */


// General hard Mapping. Make it individual for different use? customizable in the future?
/*
float UMemSys_WorkingMemoryScoringLib::GetRelationshipMultiplier(const ERelationshipType Relationship)
{
	if (Relationship == ERelationshipType::Unknown	)	{ return 1.0f; }	// same as neutral for now, debugging

	if (Relationship == ERelationshipType::Prey		)	{ return 0.1f; }
	if (Relationship == ERelationshipType::Friendly	)	{ return 0.0f; }
	if (Relationship == ERelationshipType::Neutral	)	{ return 1.0f; }	// Issue? Should I gate out friendlies in WorkMem? they have no purpose in this system
	if (Relationship == ERelationshipType::Hostile	)	{ return 1.5f; }
	if (Relationship == ERelationshipType::Predator	)	{ return 2.5f; }
	
	if (Relationship == ERelationshipType::Other	)	{ return 0.0f; }

	return 0.0f;
}
*/

