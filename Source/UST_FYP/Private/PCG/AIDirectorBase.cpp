// Fill out your copyright notice in the Description page of Project Settings.

#include "PCG/AIDirectorBase.h"

FEventSpawnConfig AAIDirectorBase::SelectSpawnConfig(float FinalIntensityScore)
{
    // Step 1: Filter to configs eligible at the current intensity score
    TArray<FEventSpawnConfig> FilteredConfigs;
    for (const FEventSpawnConfig& Entry : EventSpawnConfigs)
    {
        if (Entry.MinIntensity <= FinalIntensityScore && FinalIntensityScore <= Entry.MaxIntensity)
        {
            FilteredConfigs.Add(Entry);
        }
    }

    // Step 2: No eligible configs — return a default (EventName stays NAME_None)
    if (FilteredConfigs.IsEmpty())
    {
        return FEventSpawnConfig();
    }

    // Step 3: Sum all weights across eligible configs
    float TotalWeight = 0.0f;
    for (const FEventSpawnConfig& Entry : FilteredConfigs)
    {
        TotalWeight += Entry.Weight;
    }

    // Step 4: Pick a random point in [0, TotalWeight]
    float RandomValue = FMath::RandRange(0.0f, TotalWeight);

    // Step 5: Walk through buckets until the random value is consumed
    for (const FEventSpawnConfig& Entry : FilteredConfigs)
    {
        RandomValue -= Entry.Weight;
        if (RandomValue <= 0.0f)
        {
            return Entry;
        }
    }

    // Step 6: Safety fallback (handles floating-point edge cases)
    return FilteredConfigs.Last();
}

bool AAIDirectorBase::IsValidSpawnConfig(FEventSpawnConfig Config)
{
    return Config.EventName != NAME_None;
}
