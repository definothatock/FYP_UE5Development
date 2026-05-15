// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "WorldGeneratorTest.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(CheckHeightVariation, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(IsAggreArrayWork, Warning, All);

// ============================================================================
//                                   ENUMS
// ============================================================================

// Define Zone Types for Map Regions
UENUM(BlueprintType)
enum class EZoneType : uint8
{
    Wilderness = 0,     // Default random terrain
    CityRuins,          // Flat area for buildings
    ExtractionPoint     // Flat area for Exit Point
};

// Define Loot Rarity Tiers
UENUM(BlueprintType)
enum class ELootRarity : uint8
{
    Common = 0,
    Uncommon,
    Rare,
    Epic
};

// ============================================================================
//                                  STRUCTS
// ============================================================================

// Struct to define a single Zone's properties
USTRUCT(BlueprintType)
struct FZoneInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FVector2D Center;   // Zone center (X, Y)

    UPROPERTY(BlueprintReadOnly)
    float Radius;       // Zone radius

    UPROPERTY(BlueprintReadOnly)
    EZoneType Type;     // Zone type

    UPROPERTY(BlueprintReadOnly)
    float TargetHeight; // Target Z height (e.g., flatten city to this level)

    UPROPERTY(BlueprintReadOnly)
    TSubclassOf<AActor> ActorClassToSpawn; // Actor class to spawn at zone center
};

// Struct to define a single Foliage Type's properties
USTRUCT(BlueprintType)
struct FFoliageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMesh* Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Weight = 1.0f;

	// [NEW] Individual Density Control (0.0 ~ 1.0)
	// 1.0 = Always spawn if selected. 0.1 = 10% chance to spawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="1.0"))
	float Density = 1.0f;

	// [NEW] Min Scale (X, Y, Z)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MinScale = FVector(0.8f, 0.8f, 0.8f);

	// [NEW] Max Scale (X, Y, Z)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MaxScale = FVector(1.2f, 1.2f, 1.2f);

	// [NEW] Lock XY Axis? (True = Round trunk, False = Random XYZ)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLockXY = true;

	// [NEW] Sink amount into ground (cm) to prevent floating
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundSink = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ZOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ActorClass;

	int32 ComponentIndex = -1;
};

// Struct to define a single Zone Type's spawning configuration
USTRUCT(BlueprintType)
struct FZoneTypeConfig
{
    GENERATED_BODY()

	// Actor to spawn at zone center
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> ActorClass;

	// Number of zones of this type to generate
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count = 1;

	// Minimum zone radius
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RadiusMin = 3000.0f;

	// Maximum zone radius
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RadiusMax = 6000.0f;

	// If true, force Count = 1 regardless of Count value
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSingleton = false;

	// If true, guarantee this zone spawns the exact Count by shrinking radius and forcing placement if needed.
	// Use for gameplay-critical zones (e.g. ExtractionPoint). Optional zones may fail silently on cramped maps.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bMustSpawn = false;
};

// Struct to define a single Loot Type's properties
USTRUCT(BlueprintType)
struct FLootData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName LootName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> LootActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ELootRarity Rarity = ELootRarity::Common;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnWeight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnZOffset = 0.0f;
};

// ============================================================================
//                                CLASS DEFINITION
// ============================================================================

UCLASS()
class UST_FYP_API AWorldGeneratorTest : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWorldGeneratorTest();

	// ============================================================================
	//                          CONFIGURATION & SETTINGS
	// ============================================================================

	// --- Grid Configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Config", meta = (ExposeOnSpawn = "true"))
	int XVertexCount = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Config", meta = (ExposeOnSpawn = "true"))
	int YVertexCount = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Config", meta = (ExposeOnSpawn = "true"))
	float CellSize = 1000.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Config", meta = (ExposeOnSpawn = "true"))
	int NumOfSectionsX = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Config", meta = (ExposeOnSpawn = "true"))
	int NumOfSectionsY = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain Generation Config")
	int MeshSectionIndex0 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInterface* TerrainMaterial = nullptr;

	// --- Noise Variation Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Variation",
		meta = (UIMin = "5000.0", UIMax = "30000.0", ClampMin = "5000.0", ClampMax = "30000.0"))
	float MountainSize = 15000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Variation",
		meta = (UIMin = "500.0", UIMax = "8000.0", ClampMin = "500.0", ClampMax = "8000.0"))
	float BoulderSize = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Variation",
	meta = (UIMin = "0.0", UIMax = "5000.0"))
	float MountainVariation = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Variation",
		meta = (UIMin = "0.0", UIMax = "2000.0"))
	float BoulderVariation = 1000.0f;

	// Number of Laplacian smoothing passes (higher = smoother terrain)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Variation",
		meta = (UIMin = "0", UIMax = "5", ClampMin = "0", ClampMax = "5"))
	int32 SmoothingIterations = 2;

	// Strength of each smoothing pass (0=no effect, 1=full average with neighbors)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain Generation Variation",
		meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float SmoothingStrength = 0.5f;

	// --- Runtime Generated Random Values ---
	// (Replicated for Deterministic Generation)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Terrain Generation Variation") float MountainHeight;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Terrain Generation Variation") float BoulderHeight;

	// Random Offsets for Perlin Noise
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Terrain Generation Variation") FVector2D MountainOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Terrain Generation Variation") FVector2D BoulderOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Terrain Generation Variation") FVector2D HillOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Terrain Generation Variation") FVector2D DetailOffset;

	// --- Zone Manager Configuration ---
    UPROPERTY(BlueprintReadWrite, Category = "Zone Manager")
    TArray<FZoneInfo> GeneratedZones; // List of generated zones

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone Manager")
    TArray<FZoneTypeConfig> ZoneTypeConfigs; // Configure zone types in Editor Details Panel

	// --- Loot System Configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot System", meta = (ExposeOnSpawn = "true"))
	TArray<FLootData> LootCollection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot System", meta = (ExposeOnSpawn = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float LootDensity = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot System", meta = (ExposeOnSpawn = "true", ClampMin = "0.1"))
	float BaseLootPerPlayer = 1.5f;

	// --- Foliage Configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain|Foliage", meta = (ExposeOnSpawn = "true"))
	TArray<FFoliageInfo> FoliageCollection; // List of foliage types to spawn

	// ============================================================================
	//                               COMPONENTS & DATA
	// ============================================================================

	UPROPERTY(BlueprintReadOnly)
	UProceduralMeshComponent* TerrainMesh;

	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> AllTerrainVertices; // Cache of all generated vertices

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// ============================================================================
	//                              CORE GENERATION FUNCTIONS
	// ============================================================================

	UFUNCTION(BlueprintCallable, Category = "Terrain Generation")
	void InitializeTerrainParameters();

	UFUNCTION(BlueprintCallable)
	int GenerateTerrain(const int SectionIndexX, const int SectionIndexY);

	// [NEW] Generate Foliage based on Vertices (Called from Blueprint)
	UFUNCTION(BlueprintCallable, Category = "Terrain|Foliage")
	void GenerateFoliageFromVertices();

	// Spawn actors for all generated zones (Call after GenerateTerrain, controls spawn timing)
	UFUNCTION(BlueprintCallable, Category = "Zone Manager")
	void SpawnZoneActors();

	// Spawn loot actors at random terrain vertices (Server only, call from Blueprint)
	UFUNCTION(BlueprintCallable, Category = "Loot System")
	void SpawnLoot();

	UFUNCTION(BlueprintCallable)
	void TestNavUpdate(USceneComponent* SceneComponent);
	// ============================================================================
	//                              AI & NAVIGATION SYSTEM
	// ============================================================================

	// Force rebuild navigation mesh at runtime (fixes AI pathfinding issues)
	UFUNCTION(BlueprintCallable, Category = "Terrain|AI")
	void ForceRebuildNavigation();

	// Safely spawn AI pawns at random terrain vertices
	// EnemyClass: your BP_Enemy class
	// Count: number of enemies to spawn
	UFUNCTION(BlueprintCallable, Category = "Terrain|AI")
	void SpawnEnemies(TSubclassOf<APawn> EnemyClass, int32 Count);


	// ============================================================================
	//                              MATH & UTILITY FUNCTIONS
	// ============================================================================

	// Get exact ground height at X,Y (Includes Zone flattening logic)
	UFUNCTION(BlueprintCallable)
	float GetHeight(const FVector2D Location);

	float PerlinNoiseExtended(const FVector2D Location, const float Scale, const float Amplitude, const FVector2D offset);

private:
	// ============================================================================
	//                              INTERNAL LOGIC (PRIVATE)
	// ============================================================================

	// [NEW] Calculate pure noise height (Without Zone Influence)
	float GetBaseNoiseHeight(FVector2D Location);

    // Generate Zone locations and types
    void GenerateZones();

    // Apply Zone influence to flatten terrain (e.g. Cities)
    float ApplyZoneHeightInfluence(float OriginalHeight, FVector2D Location);

	// Helper: Calculate total weight for Foliage Selection
	float CalculateTotalFoliageWeight();

	// Helper: Initialize HISM Components for Foliage
	void InitializeFoliageComponents();

	// Store runtime created HISM Components
	UPROPERTY()
	TArray<UHierarchicalInstancedStaticMeshComponent*> FoliageComponents;
};

//Testing just to update the perforce version control