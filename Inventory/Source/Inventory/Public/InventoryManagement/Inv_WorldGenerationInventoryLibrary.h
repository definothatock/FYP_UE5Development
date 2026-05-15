#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Inv_WorldGenerationInventoryLibrary.generated.h"

class AActor;
class APlayerController;

UCLASS()
class INVENTORY_API UInv_WorldGenerationInventoryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * If SpawnedLoot has UInv_ItemComponent and manifest.bGrantToWorldGeneratorPlayer is true,
	 * grants one copy to Instigator's inventory (server only). Call after spawning world loot for the player who ran world generation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|World Generation", meta = (WorldContext = "WorldContextObject"))
	static void GrantLootToWorldGenInstigatorIfMarked(UObject* WorldContextObject, APlayerController* Instigator, AActor* SpawnedLoot);
};
