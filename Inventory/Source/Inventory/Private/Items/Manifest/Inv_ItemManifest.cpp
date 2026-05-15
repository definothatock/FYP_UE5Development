
#include "Items/Manifest/Inv_ItemManifest.h"

#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Widgets/Composite/Inv_CompositeBase.h"

UInv_InventoryItem* FInv_ItemManifest::Manifest(UObject* NewOuter)
{
	UInv_InventoryItem* Item = NewObject<UInv_InventoryItem>(NewOuter, UInv_InventoryItem::StaticClass());
	Item->SetItemManifest(*this);
	for (auto& Fragment : Item->GetItemManifestMutable().GetFragmentsMutable())
	{
		Fragment.GetMutable().Manifest();
	}
	ClearFragments();
	
	return Item;
}

UTexture2D* FInv_ItemManifest::ResolveInventoryIconTexture() const
{
	const FGameplayTag IconTag = FGameplayTag::RequestGameplayTag(TEXT("FragmentTags.IconFragment"), false);
	if (IconTag.IsValid())
	{
		if (const FInv_ImageFragment* Tagged = GetFragmentOfTypeWithTag<FInv_ImageFragment>(IconTag))
		{
			if (IsValid(Tagged->GetIcon()))
			{
				return Tagged->GetIcon();
			}
		}
	}
	if (const FInv_ImageFragment* AnyImage = GetFragmentOfType<FInv_ImageFragment>())
	{
		if (IsValid(AnyImage->GetIcon()))
		{
			return AnyImage->GetIcon();
		}
	}
	if (const FInv_EquipmentFragment* Equip = GetFragmentOfType<FInv_EquipmentFragment>())
	{
		if (IsValid(Equip->GetInventoryIcon()))
		{
			return Equip->GetInventoryIcon();
		}
	}
	return nullptr;
}

void FInv_ItemManifest::AssimilateInventoryFragments(UInv_CompositeBase* Composite) const
{
	const auto& InventoryItemFragments = GetAllFragmentsOfType<FInv_InventoryItemFragment>();
	for (const auto* Fragment : InventoryItemFragments)
	{
		Composite->ApplyFunction([Fragment](UInv_CompositeBase* Widget)
		{
			Fragment->Assimilate(Widget);
		});
	}
}
///////////
void FInv_ItemManifest::SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation, FInv_ItemManifest ItemManifest)
{
	if (!IsValid(PickupActorClass) || !IsValid(WorldContextObject)) return;

	AActor* SpawnedActor = WorldContextObject->GetWorld()->SpawnActor<AActor>(PickupActorClass, SpawnLocation, SpawnRotation);
	if (!IsValid(SpawnedActor)) return;

	// Set the item manifest, item category, item type, etc.
	UInv_ItemComponent* ItemComp = SpawnedActor->FindComponentByClass<UInv_ItemComponent>();
	check(ItemComp);

	// ItemComp->InitItemManifest(*this);
	ItemComp->InitItemManifest(ItemManifest);
}

void FInv_ItemManifest::ClearFragments()
{
	for (auto& Fragment : Fragments)
	{
		Fragment.Reset();
	}
	Fragments.Empty();
}
