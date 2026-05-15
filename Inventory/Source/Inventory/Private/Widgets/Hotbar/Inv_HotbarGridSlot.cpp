#include "Widgets/Hotbar/Inv_HotbarGridSlot.h"

#include "Components/Image.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Inv_InventoryItem.h"

void UInv_HotbarGridSlot::NativeConstruct()
{
	Super::NativeConstruct();
	InventoryComponent = TWeakObjectPtr<UInv_InventoryComponent>(
		UInv_InventoryStatics::GetInventoryComponent(GetOwningPlayer()));
	HotbarComponent = TWeakObjectPtr<UInv_HotbarComponent>(
		UInv_InventoryStatics::GetHotbarComponent(GetOwningPlayer()));
	if (HotbarComponent.IsValid()
		&& !HotbarComponent->OnHotbarUpdated.IsAlreadyBound(this, &ThisClass::UpdateHotbarSlot))
		HotbarComponent->OnHotbarUpdated.AddDynamic(this, &ThisClass::UpdateHotbarSlot);
}

void UInv_HotbarGridSlot::NativeDestruct()
{
	Super::NativeDestruct();
	if (HotbarComponent.IsValid())
	{
		HotbarComponent->OnHotbarUpdated.RemoveAll(this);
	}
}

void UInv_HotbarGridSlot::InitializeHotbarSlot(UInv_InventoryComponent* IC, UInv_HotbarComponent* HC)
{
	InventoryComponent = TWeakObjectPtr<UInv_InventoryComponent>(IC);
 	HotbarComponent = TWeakObjectPtr<UInv_HotbarComponent>(HC);
	if (HotbarComponent.IsValid()
		&& !HotbarComponent->OnHotbarUpdated.IsAlreadyBound(this, &ThisClass::UpdateHotbarSlot))
 		HotbarComponent->OnHotbarUpdated.AddDynamic(this, &ThisClass::UpdateHotbarSlot);
}

void UInv_HotbarGridSlot::UpdateHotbarSlot(int32 BroadcastSlotIndex, UInv_InventoryItem* Item)
{
	if (!HotbarComponent.IsValid()) return;
	if (BroadcastSlotIndex != SlotIndex) return;
	if (!HotbarComponent->IsValidSlotIndex(BroadcastSlotIndex)) return;
	if (!Item)
	{
		Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	
	CreateImage(Item);
}

void UInv_HotbarGridSlot::CreateImage(const UInv_InventoryItem* Item) const
{
	if (!IsValid(Item))
	{
		return;
	}

	if (auto* Icon = Item->GetItemManifest().ResolveInventoryIconTexture(); IsValid(Icon))
	{
		Image_Icon->SetBrushFromTexture(Icon);
		Image_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}
