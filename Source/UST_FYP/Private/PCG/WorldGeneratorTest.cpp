// Fill out your copyright notice in the Description page of Project Settings.

#include "PCG/WorldGeneratorTest.h"
#include "KismetProceduralMeshLibrary.h"
#include "NavigationSystem.h"
#include "AIController.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Math/UnrealMathUtility.h"
#include "GameFramework/GameStateBase.h"

DEFINE_LOG_CATEGORY(CheckHeightVariation);

// ============================================================================
//                               LIFECYCLE FUNCTIONS
// ============================================================================

// Sets default values
AWorldGeneratorTest::AWorldGeneratorTest()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // Enable Networking

    TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
    TerrainMesh->SetupAttachment(GetRootComponent());

	// Enable Collision for Player interaction
	TerrainMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TerrainMesh->SetCanEverAffectNavigation(true);

    // Disable Async Cooking to ensure collision is ready immediately (if needed),
	// or keep enabled for performance (but requires handling spawn delay).
    TerrainMesh->bUseAsyncCooking = false;
}

// Called when the game starts or when spawned
void AWorldGeneratorTest::BeginPlay()
{
   Super::BeginPlay();
}

// Called every frame
void AWorldGeneratorTest::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// ============================================================================
//                          TERRAIN GENERATION LOGIC
// ============================================================================

void AWorldGeneratorTest::InitializeTerrainParameters()
{
   AllTerrainVertices.Empty();
   MeshSectionIndex0 = 0;

   float MountainMin = 10000.0f;
   float MountainMax = 20000.0f;
   float BoulderMin = 1000.0f;
   float BoulderMax = 3000.0f;

   MountainHeight = FMath::FRandRange(MountainMin, MountainMax);
   BoulderHeight = FMath::FRandRange(BoulderMin, BoulderMax);

   // Generate Random Offsets for Noise
   MountainOffset = FVector2D(FMath::FRandRange(-50000.0f, 50000.0f), FMath::FRandRange(-50000.0f, 50000.0f));
   BoulderOffset = FVector2D(FMath::FRandRange(-50000.0f, 50000.0f), FMath::FRandRange(-50000.0f, 50000.0f));
   HillOffset = FVector2D(FMath::FRandRange(-50000.0f, 50000.0f), FMath::FRandRange(-50000.0f, 50000.0f));
   DetailOffset = FVector2D(FMath::FRandRange(-50000.0f, 50000.0f), FMath::FRandRange(-50000.0f, 50000.0f));

   UE_LOG(CheckHeightVariation, Warning, TEXT("Initialized - MountainHeight: %f (Size: %f ± %f), Offset: %s"),
       MountainHeight, MountainSize, MountainVariation, *MountainOffset.ToString());
   UE_LOG(CheckHeightVariation, Warning, TEXT("Initialized - BoulderHeight: %f (Size: %f ± %f), Offset: %s"),
       BoulderHeight, BoulderSize, BoulderVariation, *BoulderOffset.ToString());

   // After initializing random parameters, generate the Zones (Cities/Extraction)
   GenerateZones();
}

int AWorldGeneratorTest::GenerateTerrain(const int SectionIndexX, const int SectionIndexY)
{
    FVector Offset = FVector(SectionIndexX*(XVertexCount-1), SectionIndexY*(YVertexCount-1), 0.f) * CellSize;

    TArray<FVector> Vertices;
    FVector Vertex;

    TArray<FVector2D> UVs;
    FVector2D UV;

    TArray<FVector> Normals;
    TArray<FProcMeshTangent> Tangents;

    TArray<int32>Triangles;

    int DrawnMeshSection;

    // 1. Generate Vertices and UVs
    for(int32 iVY = -1; iVY <= YVertexCount; iVY++)
    {
       for(int32 iVX = -1 ; iVX <= XVertexCount; iVX++)
       {
          // Vertex Calculation
          Vertex.X = iVX * CellSize + Offset.X;
          Vertex.Y = iVY * CellSize + Offset.Y;
          Vertex.Z = GetHeight(FVector2D(Vertex.X, Vertex.Y)); // Uses Adaptive Height Logic
          Vertices.Add(Vertex);

          // UV Mapping
          UV.X = (iVX + (SectionIndexX * (XVertexCount -1 )))*CellSize/100;
          UV.Y = (iVY + (SectionIndexY * (YVertexCount -1 )))*CellSize/100;
          UVs.Add(UV);
       }
    }

    // 2. Laplacian Smoothing Pass (reduces jagged/angular terrain)
    int32 GridWidth  = XVertexCount + 2;
    int32 GridHeight = YVertexCount + 2;
    for (int32 Iter = 0; Iter < SmoothingIterations; Iter++)
    {
        TArray<float> SmoothedZ;
        SmoothedZ.SetNum(Vertices.Num());
        for (int32 iY = 0; iY < GridHeight; iY++)
        {
            for (int32 iX = 0; iX < GridWidth; iX++)
            {
                int32 Idx = iY * GridWidth + iX;
                float AvgZ = 0.0f;
                int32 NeighborCount = 0;
                if (iX > 0)            { AvgZ += Vertices[Idx - 1].Z;          NeighborCount++; }
                if (iX < GridWidth - 1){ AvgZ += Vertices[Idx + 1].Z;          NeighborCount++; }
                if (iY > 0)            { AvgZ += Vertices[Idx - GridWidth].Z;  NeighborCount++; }
                if (iY < GridHeight- 1){ AvgZ += Vertices[Idx + GridWidth].Z;  NeighborCount++; }
                if (NeighborCount > 0) AvgZ /= NeighborCount;
                SmoothedZ[Idx] = FMath::Lerp(Vertices[Idx].Z, AvgZ, SmoothingStrength);
            }
        }
        for (int32 i = 0; i < Vertices.Num(); i++)
        {
            int32 iX = i % GridWidth;
            int32 iY = i / GridWidth;
            // Only apply to interior vertices — seam/overlap vertices keep original GetHeight() value
            if (iX >= 2 && iX <= GridWidth - 3 && iY >= 2 && iY <= GridHeight - 3)
            {
                Vertices[i].Z = SmoothedZ[i];
            }
        }
    }

    // 3. Generate Triangles (Grid Topology)
    if (Triangles.Num() == 0)
    {
       for(int32 iTY = 0; iTY <= YVertexCount; iTY++)
       {
          for(int32 iTX = 0; iTX <= XVertexCount; iTX++)
          {
             Triangles.Add(iTX + iTY * (XVertexCount+2));
             Triangles.Add(iTX + (iTY+1)*(XVertexCount+2));
             Triangles.Add(iTX + iTY*(XVertexCount+2)+1);

             Triangles.Add(iTX + (iTY+1)*(XVertexCount+2));
             Triangles.Add(iTX + (iTY+1)*(XVertexCount+2)+1);
             Triangles.Add(iTX + iTY*(XVertexCount+2)+1);
          }
       }
    }

    // 3. Subset Mesh Calculation (To prevent seams)
    int VertexIndex = 0;

    TArray<FVector> SubVertices;
    TArray<FVector2D> SubUVs;
    TArray<int32> SubTriangles;
    TArray<FVector> SubNormals;
    TArray<FProcMeshTangent> SubTangents;

    // Calculate Normals/Tangents automatically
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);

    // Subset Vertices and UVs
    for (int32 iVY = -1; iVY <= YVertexCount; iVY++)
    {
       for (int32 iVX = -1 ; iVX <= XVertexCount; iVX++)
       {
          if(-1<iVY && iVY < YVertexCount && -1<iVX && iVX < XVertexCount)
          {
             SubVertices.Add(Vertices[VertexIndex]);
             SubUVs.Add(UVs[VertexIndex]);
             SubNormals.Add(Normals[VertexIndex]);
             SubTangents.Add(Tangents[VertexIndex]);
          }
          VertexIndex++;
       }
    }

	// Cache Vertices for later use (e.g., Foliage Spawning)
	AllTerrainVertices.Append(SubVertices);

    // Subset Triangles
    if (SubTriangles.Num() == 0)
    {
       for(int32 iTY = 0; iTY <= YVertexCount-2; iTY++)
       {
          for(int32 iTX = 0; iTX <= XVertexCount-2; iTX++)
          {
             SubTriangles.Add(iTX + iTY* XVertexCount);
             SubTriangles.Add(iTX + iTY * XVertexCount + XVertexCount);
             SubTriangles.Add(iTX + iTY * XVertexCount+1);

             SubTriangles.Add(iTX + iTY * XVertexCount + XVertexCount);
             SubTriangles.Add(iTX + iTY * XVertexCount + XVertexCount +1);
             SubTriangles.Add(iTX + iTY * XVertexCount +1);
          }
       }
    }

    // 4. Create Mesh Section
    TerrainMesh->CreateMeshSection(MeshSectionIndex0, SubVertices, SubTriangles, SubNormals, SubUVs, TArray<FColor>(), SubTangents, true);
    if (TerrainMesh)
    {
       TerrainMesh->SetMaterial(MeshSectionIndex0, TerrainMaterial);
       UE_LOG(CheckHeightVariation, Warning, TEXT("Navigation system has updated! MountainHeight: %f"), MountainHeight);
       UE_LOG(CheckHeightVariation, Warning, TEXT("Navigation system has updated! BoulderHeight: %f"), BoulderHeight);
    }
    DrawnMeshSection = MeshSectionIndex0;
    MeshSectionIndex0++;

    return DrawnMeshSection;
}

// ============================================================================
//                          HEIGHT CALCULATION LOGIC
// ============================================================================

float AWorldGeneratorTest::GetHeight(const FVector2D Location)
{
	// 1. Calculate Base Noise Height (Mountains/Hills)
	float OriginalZ = GetBaseNoiseHeight(Location);

	// 2. Apply Zone Influence (Flatten Cities/Extraction Points)
	return ApplyZoneHeightInfluence(OriginalZ, Location);
}

// Calculates the raw Perlin Noise height without Zone modification
float AWorldGeneratorTest::GetBaseNoiseHeight(FVector2D Location)
{
	return
	   PerlinNoiseExtended(Location, .00001f, MountainHeight, MountainOffset) +
	   PerlinNoiseExtended(Location, .0001f, BoulderHeight, BoulderOffset) +
	   PerlinNoiseExtended(Location, .001f, 500, HillOffset);
	// Removed .01f detail noise: period (~100 units) smaller than CellSize, causes aliasing/jagged edges
}

float AWorldGeneratorTest::PerlinNoiseExtended(const FVector2D Location, const float Scale, const float Amplitude,
    const FVector2D offset)
{
    return FMath::PerlinNoise2D(Location*Scale+FVector2D(0.1f, 0.1f) + offset)* Amplitude;
}

// ============================================================================
//                          ZONE MANAGER LOGIC
// ============================================================================

void AWorldGeneratorTest::GenerateZones()
{
    GeneratedZones.Empty();

    float MapWidth = XVertexCount * CellSize * NumOfSectionsX;
    float MapHeight = YVertexCount * CellSize * NumOfSectionsY;
    float Buffer = 2000.0f;
    const int32 MaxAttempts = 50;
    const int32 MaxShrinkRounds = 3;
    const float ShrinkFactor = 0.7f;

    // Two-pass placement: bMustSpawn zones first (best chance on empty map), then optional zones
    for (int32 Pass = 0; Pass < 2; Pass++)
    {
        const bool bPassForMustSpawn = (Pass == 0);

        for (const FZoneTypeConfig& Config : ZoneTypeConfigs)
        {
            if (Config.bMustSpawn != bPassForMustSpawn) continue;

            int32 SpawnCount = Config.bSingleton ? 1 : Config.Count;
            for (int32 i = 0; i < SpawnCount; i++)
            {
                float InitialRadius = FMath::FRandRange(Config.RadiusMin, Config.RadiusMax);
                float CurrentRadius = InitialRadius;
                bool bPlaced = false;
                int32 ShrinkRound = 0;

                // Attempt loop with fallback radius shrinking for must-spawn zones
                while (!bPlaced && ShrinkRound <= MaxShrinkRounds)
                {
                    for (int32 Attempt = 0; Attempt < MaxAttempts; Attempt++)
                    {
                        FVector2D CandidateCenter(
                            FMath::FRandRange(Buffer, MapWidth - Buffer),
                            FMath::FRandRange(Buffer, MapHeight - Buffer)
                        );

                        bool bOverlaps = false;
                        for (const FZoneInfo& ExistingZone : GeneratedZones)
                        {
                            float MinAllowedDist = CurrentRadius + ExistingZone.Radius;
                            if (FVector2D::Distance(CandidateCenter, ExistingZone.Center) < MinAllowedDist)
                            {
                                bOverlaps = true;
                                break;
                            }
                        }

                        if (!bOverlaps)
                        {
                            FZoneInfo NewZone;
                            NewZone.Center = CandidateCenter;
                            NewZone.Radius = CurrentRadius;
                            NewZone.Type = EZoneType::CityRuins;
                            NewZone.ActorClassToSpawn = Config.ActorClass;
                            NewZone.TargetHeight = GetBaseNoiseHeight(NewZone.Center);
                            GeneratedZones.Add(NewZone);
                            bPlaced = true;

                            if (ShrinkRound > 0)
                            {
                                UE_LOG(CheckHeightVariation, Warning, TEXT("[MustSpawn] Zone #%d placed after shrink x%d (radius %.0f -> %.0f)"),
                                    i, ShrinkRound, InitialRadius, CurrentRadius);
                            }
                            break;
                        }
                    }

                    // Only must-spawn zones get radius-shrink retries; optional zones exit here
                    if (!bPlaced && Config.bMustSpawn && ShrinkRound < MaxShrinkRounds)
                    {
                        ShrinkRound++;
                        CurrentRadius *= ShrinkFactor;
                    }
                    else if (!bPlaced)
                    {
                        break;
                    }
                }

                // Last resort for must-spawn: force placement allowing partial overlap
                if (!bPlaced && Config.bMustSpawn)
                {
                    FVector2D ForcedCenter(
                        FMath::FRandRange(Buffer, MapWidth - Buffer),
                        FMath::FRandRange(Buffer, MapHeight - Buffer)
                    );

                    FZoneInfo NewZone;
                    NewZone.Center = ForcedCenter;
                    NewZone.Radius = CurrentRadius;
                    NewZone.Type = EZoneType::CityRuins;
                    NewZone.ActorClassToSpawn = Config.ActorClass;
                    NewZone.TargetHeight = GetBaseNoiseHeight(NewZone.Center);
                    GeneratedZones.Add(NewZone);
                    bPlaced = true;

                    UE_LOG(CheckHeightVariation, Warning, TEXT("[MustSpawn] Zone #%d FORCED (overlap allowed) radius=%.0f"),
                        i, CurrentRadius);
                }

                if (!bPlaced)
                {
                    UE_LOG(CheckHeightVariation, Warning, TEXT("[Optional] Zone #%d skipped after %d attempts."), i, MaxAttempts);
                }
            }
        }
    }
}

void AWorldGeneratorTest::SpawnZoneActors()
{
    UWorld* World = GetWorld();
    if (!World) return;

    for (const FZoneInfo& Zone : GeneratedZones)
    {
        if (!Zone.ActorClassToSpawn) continue;

        FVector SpawnLocation(Zone.Center.X, Zone.Center.Y, Zone.TargetHeight);
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        World->SpawnActor<AActor>(Zone.ActorClassToSpawn, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
    }
}

float AWorldGeneratorTest::ApplyZoneHeightInfluence(float OriginalHeight, FVector2D Location)
{
	float FinalHeight = OriginalHeight;

	for (const FZoneInfo& Zone : GeneratedZones)
	{
		float Dist = FVector2D::Distance(Location, Zone.Center);

		// Check if inside Zone Radius
		if (Dist < Zone.Radius)
		{
			if (Zone.Type == EZoneType::CityRuins || Zone.Type == EZoneType::ExtractionPoint)
			{
				// Smooth blending logic
				float BlendStartRadius = Zone.Radius * 0.7f;

				if (Dist < BlendStartRadius)
				{
					// Core Area: Completely Flat
					return Zone.TargetHeight;
				}
				else
				{
					// Edge Area: Blend from Flat to Mountain
					float Alpha = (Dist - BlendStartRadius) / (Zone.Radius - BlendStartRadius);
					Alpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

					// Lerp between Target and Original Height
					return FMath::Lerp(Zone.TargetHeight, OriginalHeight, Alpha);
				}
			}
		}
	}
	return FinalHeight;
}

// ============================================================================
//                          FOLIAGE SYSTEM LOGIC (HISM)
// ============================================================================

void AWorldGeneratorTest::InitializeFoliageComponents()
{
	// Clear old components
	for (auto Comp : FoliageComponents)
	{
		if (Comp) Comp->DestroyComponent();
	}
	FoliageComponents.Empty();

	// Create HISM for each Foliage Type in Collection
	for (int32 i = 0; i < FoliageCollection.Num(); i++)
	{
		FName CompName = FName(*FString::Printf(TEXT("FoliageHISM_%d"), i));
		UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, CompName);

		if (FoliageCollection[i].Mesh)
		{
			HISM->SetStaticMesh(FoliageCollection[i].Mesh);
		}

		HISM->RegisterComponent();
		HISM->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

		// [IMPORTANT] Enable Collision
		HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HISM->SetCollisionProfileName(TEXT("BlockAll"));

		FoliageComponents.Add(HISM);
	}
}

float AWorldGeneratorTest::CalculateTotalFoliageWeight()
{
	float Total = 0.0f;
	for (const FFoliageInfo& Info : FoliageCollection)
	{
		Total += Info.Weight;
	}
	return Total;
}

void AWorldGeneratorTest::GenerateFoliageFromVertices()
{
	  // 1. Safety Checks & Initialization
    if (AllTerrainVertices.Num() == 0) return;
    InitializeFoliageComponents();
    float TotalWeight = CalculateTotalFoliageWeight();
    if (TotalWeight <= 0.0f) return;

    // 2. Setup Variables (BatchTransforms & Seed)
    TArray<TArray<FTransform>> BatchTransforms;
    BatchTransforms.SetNum(FoliageCollection.Num());
    int32 GlobalSeed = (int32)MountainOffset.X;

    // 3. Loop through all vertices
    for (const FVector& Vertex : AllTerrainVertices)
    {
        // --- Zone Filtering (Skip Cities) ---
        bool bIsInsideZone = false;
        FVector2D VertexPos(Vertex.X, Vertex.Y);
        for (const FZoneInfo& Zone : GeneratedZones)
        {
            if (FVector2D::Distance(VertexPos, Zone.Center) < Zone.Radius * 0.9f)
            {
                bIsInsideZone = true; break;
            }
        }
        if (bIsInsideZone) continue;
        // ------------------------------------

        // --- Hashing Algorithm (Deterministic Random) ---
        int32 Xi = (int32)Vertex.X;
        int32 Yi = (int32)Vertex.Y;
        int32 HashX = Xi * 1327;
        int32 HashY = Yi * 29311;
        int32 VertexSeed = (HashX ^ HashY) ^ GlobalSeed;
        FRandomStream Stream(VertexSeed);

        // --- Weighted Selection Logic ---
        float Pick = Stream.FRandRange(0.0f, TotalWeight);
        int32 SelectedIndex = -1;
        float CurrentWeightSum = 0.0f;

        for (int32 i = 0; i < FoliageCollection.Num(); i++)
        {
            CurrentWeightSum += FoliageCollection[i].Weight;
            if (Pick <= CurrentWeightSum)
            {
                SelectedIndex = i;
                break;
            }
        }

        // --- Placement Logic ---
        if (SelectedIndex != -1 && BatchTransforms.IsValidIndex(SelectedIndex))
        {
            FFoliageInfo& Info = FoliageCollection[SelectedIndex];

            // [A] Individual Density Check
            // If random value > Density, skip this spawn (creates sparsity)
            if (Stream.FRand() > Info.Density) continue;

            // [B] Calculate Position with Offset
            float OffsetX = Stream.FRandRange(-200.0f, 200.0f);
            float OffsetY = Stream.FRandRange(-200.0f, 200.0f);
            float NewX = Vertex.X + OffsetX;
            float NewY = Vertex.Y + OffsetY;

            // Offset Zone Check (Double check to prevent spawning in zones)
            bool bOffsetInZone = false;
             for (const FZoneInfo& Zone : GeneratedZones)
            {
                if (FVector2D::Distance(FVector2D(NewX, NewY), Zone.Center) < Zone.Radius * 0.9f)
                {
                    bOffsetInZone = true; break;
                }
            }
            if (bOffsetInZone) continue;

            // [C] Ground Sinking Logic (Fix Floating)
            float AccurateZ = Vertex.Z;

            // Calculate Final Z: Height + Offset - Sink Amount
            float FinalZ = AccurateZ + Info.ZOffset - Info.GroundSink;

            FVector FinalLocation(NewX, NewY, FinalZ);

            // [D] Advanced Scale Variation
            FVector FinalScale;
            if (Info.bLockXY)
            {
                // Lock XY: Keep the trunk round, scale Z independently
                float ScaleXY = Stream.FRandRange(Info.MinScale.X, Info.MaxScale.X);
                float ScaleZ = Stream.FRandRange(Info.MinScale.Z, Info.MaxScale.Z);
                FinalScale = FVector(ScaleXY, ScaleXY, ScaleZ);
            }
            else
            {
                // Unlock XY: Fully random scaling (Good for rocks)
                FinalScale = FVector(
                    Stream.FRandRange(Info.MinScale.X, Info.MaxScale.X),
                    Stream.FRandRange(Info.MinScale.Y, Info.MaxScale.Y),
                    Stream.FRandRange(Info.MinScale.Z, Info.MaxScale.Z)
                );
            }

            // [E] Random Rotation
            float RandYaw = Stream.FRandRange(0.0f, 360.0f);

            // [F] Construct Transform
            FTransform Transform;
            Transform.SetLocation(FinalLocation);
            Transform.SetRotation(FQuat(FRotator(0.0f, RandYaw, 0.0f)));
            Transform.SetScale3D(FinalScale);

            // [G] Spawn or Batch based on input type
            if (Info.ActorClass)
            {
                // BP input: spawn actor directly using full transform (includes Scale)
                FActorSpawnParameters SpawnParams;
                SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
                GetWorld()->SpawnActor<AActor>(Info.ActorClass, Transform, SpawnParams);
            }
            else
            {
                // SM input: add to HISM batch as before
                BatchTransforms[SelectedIndex].Add(Transform);
            }
        }
    }

    // 4. Batch Submit to HISM Components
    for (int32 i = 0; i < FoliageComponents.Num(); i++)
    {
        if (FoliageComponents[i] && BatchTransforms.IsValidIndex(i))
        {
            if (BatchTransforms[i].Num() > 0)
            {
                FoliageComponents[i]->AddInstances(BatchTransforms[i], false);
                FoliageComponents[i]->MarkRenderStateDirty();
                FoliageComponents[i]->UpdateBounds();
            }
        }
    }
}

// ============================================================================
//                          AI & NAVIGATION LOGIC
// ============================================================================

void AWorldGeneratorTest::ForceRebuildNavigation()
{
    if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        // Force runtime navigation rebuild
        NavSys->Build();
        UE_LOG(CheckHeightVariation, Warning, TEXT("C++ Force Rebuild Navigation Triggered!"));
    }
}

void AWorldGeneratorTest::SpawnEnemies(TSubclassOf<APawn> EnemyClass, int32 Count)
{
	if (!EnemyClass || AllTerrainVertices.Num() == 0) return;

	for (int32 i = 0; i < Count; i++)
	{
		int32 RandomIndex = FMath::RandRange(0, AllTerrainVertices.Num() - 1);
		FVector SelectedVertex = AllTerrainVertices[RandomIndex];


		FVector SpawnLocation(SelectedVertex.X, SelectedVertex.Y, GetHeight(FVector2D(SelectedVertex.X, SelectedVertex.Y)) + 150.0f);

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APawn* NewAI = GetWorld()->SpawnActor<APawn>(EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

		if (NewAI)
		{
			NewAI->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

			// 2. If no Controller assigned yet, force spawn a default one
			if (NewAI->GetController() == nullptr)
			{
				NewAI->SpawnDefaultController();
			}

			// 3. Verify Controller assignment (Debug Log)
			if (NewAI->GetController())
			{
				UE_LOG(LogTemp, Warning, TEXT("AI Spawned and POSSESSED successfully at %s"), *SpawnLocation.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("AI Spawned but FAILED to create Controller!"));
			}
		}
	}

	UE_LOG(CheckHeightVariation, Warning, TEXT("Spawned %d Enemies. Check if they have controllers!"), Count);
}


// ============================================================================
//                          LOOT SYSTEM LOGIC
// ============================================================================

void AWorldGeneratorTest::SpawnLoot()
{
    // --- Safety Checks ---
    if (!HasAuthority()) return;
    if (AllTerrainVertices.Num() == 0) return;
    if (LootCollection.Num() == 0) return;

    // --- Calculate Total Spawn Weight ---
    float TotalWeight = 0.0f;
    for (const FLootData& Loot : LootCollection)
    {
        TotalWeight += Loot.SpawnWeight;
    }
    if (TotalWeight <= 0.0f) return;

    // --- Calculate Target Loot Count ---
    int32 PlayerCount = 1;
    if (AGameStateBase* GS = GetWorld()->GetGameState())
    {
        int32 Num = GS->PlayerArray.Num();
        if (Num > 0) PlayerCount = Num;
    }

    int32 TargetLootCount = FMath::RoundToInt(
        AllTerrainVertices.Num() * LootDensity * (BaseLootPerPlayer * PlayerCount)
    );
    TargetLootCount = FMath::Max(1, TargetLootCount);

    // --- Shuffle Vertices (Fisher-Yates) ---
    TArray<int32> ShuffledIndices;
    ShuffledIndices.Reserve(AllTerrainVertices.Num());
    for (int32 i = 0; i < AllTerrainVertices.Num(); i++)
    {
        ShuffledIndices.Add(i);
    }
    for (int32 i = ShuffledIndices.Num() - 1; i > 0; i--)
    {
        int32 j = FMath::RandRange(0, i);
        ShuffledIndices.Swap(i, j);
    }

    // --- Main Loop (iterate shuffled indices) ---
    int32 SpawnedCount = 0;
    UWorld* World = GetWorld();

    for (int32 Idx : ShuffledIndices)
    {
        if (SpawnedCount >= TargetLootCount) break;

        const FVector& Vertex = AllTerrainVertices[Idx];
        FVector2D VertexPos(Vertex.X, Vertex.Y);

        // [A] Zone Filtering - skip vertices inside any zone
        bool bInsideZone = false;
        for (const FZoneInfo& Zone : GeneratedZones)
        {
            if (FVector2D::Distance(VertexPos, Zone.Center) < Zone.Radius * 0.9f)
            {
                bInsideZone = true;
                break;
            }
        }
        if (bInsideZone) continue;

        // [B] Weighted Random Selection
        float Roll = FMath::FRandRange(0.0f, TotalWeight);
        float Accumulated = 0.0f;
        const FLootData* SelectedLoot = nullptr;
        for (const FLootData& Loot : LootCollection)
        {
            Accumulated += Loot.SpawnWeight;
            if (Roll <= Accumulated)
            {
                SelectedLoot = &Loot;
                break;
            }
        }
        if (!SelectedLoot) continue;

        // [C] Validate LootActorClass
        if (!SelectedLoot->LootActorClass) continue;

        // [D] Calculate Spawn Location (Z already pre-computed in GenerateTerrain)
        FVector SpawnLocation = Vertex + FVector(0.0f, 0.0f, SelectedLoot->SpawnZOffset);

        // [E] Spawn Actor
        float RandomYaw = FMath::FRandRange(0.0f, 360.0f);
        FRotator SpawnRotation(90.0f, RandomYaw, 0.0f); // Pitch=90 lays actor flat, random Yaw prevents uniform look
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AActor* Spawned = World->SpawnActor<AActor>(SelectedLoot->LootActorClass, SpawnLocation, SpawnRotation, SpawnParams);
        if (Spawned) SpawnedCount++;
    }

    // --- Final Log ---
    UE_LOG(LogTemp, Warning, TEXT("SpawnLoot Complete: %d/%d loot spawned | Players: %d | Target: %d"),
        SpawnedCount, TargetLootCount, PlayerCount, TargetLootCount);
}

// ============================================================================
//                              UTILITIES
// ============================================================================

void AWorldGeneratorTest::TestNavUpdate(USceneComponent* SceneComponent) {
   if (SceneComponent->IsRegistered())
   {
      if (SceneComponent->GetWorld() != nullptr)
      {
         FNavigationSystem::UpdateComponentData(*SceneComponent);
         UE_LOG(CheckHeightVariation,Warning,TEXT("Navigation system has updated!"));
      }
   }
}

//Testing, just for update the perforce version control