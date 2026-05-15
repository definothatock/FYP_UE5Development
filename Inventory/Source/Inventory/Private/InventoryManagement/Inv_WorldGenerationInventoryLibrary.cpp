#include "InventoryManagement/Inv_WorldGenerationInventoryLibrary.h"

#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Manifest/Inv_ItemManifest.h"
#include "Engine/World.h"

void UInv_WorldGenerationInventoryLibrary::GrantLootToWorldGenInstigatorIfMarked(
	UObject* WorldContextObject,
	APlayerController* Instigator,
	AActor* SpawnedLoot)
{
	if (!WorldContextObject || !WorldContextObject->GetWorld()) return;
	UWorld* World = WorldContextObject->GetWorld();
	if (World->GetNetMode() == NM_Client) return;
	if (!IsValid(Instigator) || !IsValid(SpawnedLoot)) return;

	UInv_ItemComponent* ItemComp = SpawnedLoot->FindComponentByClass<UInv_ItemComponent>();
	if (!IsValid(ItemComp)) return;

	const FInv_ItemManifest ManifestCopy = ItemComp->GetItemManifest();
	if (!ManifestCopy.bGrantToWorldGeneratorPlayer) return;

	UInv_InventoryComponent* Inv = Instigator->FindComponentByClass<UInv_InventoryComponent>();
	if (!IsValid(Inv)) return;

	Inv->AuthorityGrantItemFromManifest(ManifestCopy);
}
