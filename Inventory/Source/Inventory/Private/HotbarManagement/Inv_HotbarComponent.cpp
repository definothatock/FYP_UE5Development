#include "HotbarManagement/Inv_HotbarComponent.h"

#include "EquipmentManagement/Components/Inv_EquipmentComponent.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "Types/Inv_GridTypes.h"

namespace
{
/** Hotbar auto-fill: equipment fragment, equippable/consumable, craftable, or any manifest with a valid ItemType tag (common for data-driven startup items). */
bool InvShouldAutoAssignItemToHotbar(const UInv_InventoryItem* Item, const bool bHotbarHasAnyOccupiedSlot)
{
	if (!IsValid(Item))
	{
		return false;
	}
	if (Item->IsExcludedFromHotbarAutoAssign())
	{
		return false;
	}
	// First pickup(s): hotbar is empty, so show the item on the bar even if tags/category are not set up yet.
	if (!bHotbarHasAnyOccupiedSlot)
	{
		return true;
	}
	const FInv_ItemManifest& M = Item->GetItemManifest();
	if (M.GetFragmentOfType<FInv_EquipmentFragment>())
	{
		return true;
	}
	if (Item->IsEquippable() || Item->IsConsumable())
	{
		return true;
	}
	if (M.GetItemCategory() == EInv_ItemCategory::Craftable)
	{
		return true;
	}
	return M.GetItemType().IsValid();
}
} // namespace

UInv_HotbarComponent::UInv_HotbarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	HotbarSlots.SetNum(MaxHotbarSlots);
}

void UInv_HotbarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, HotbarSlots);
}

void UInv_HotbarComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeOwner(Cast<APlayerController>(GetOwner()));
}

void UInv_HotbarComponent::InitializeOwner(APlayerController* PlayerController)
{
	if (IsValid(PlayerController))
	{
		OwningPlayerController = PlayerController;
	}
	InitInventoryComponent();
}

void UInv_HotbarComponent::InitInventoryComponent()
{
	InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(OwningPlayerController.Get());
	if (!InventoryComponent.IsValid()) return;

	if (!InventoryComponent->OnItemAdded.IsAlreadyBound(this, &ThisClass::OnItemAdded))
	{
		InventoryComponent->OnItemAdded.AddDynamic(this, &ThisClass::OnItemAdded);
	}

	if (!InventoryComponent->OnItemRemoved.IsAlreadyBound(this, &ThisClass::OnItemRemoved))
	{
		InventoryComponent->OnItemRemoved.AddDynamic(this, &ThisClass::OnItemRemoved);
	}

	if (!InventoryComponent->OnHotbarSlotChanged.IsAlreadyBound(this, &ThisClass::OnHotbarSlotChanged))
	{
		InventoryComponent->OnHotbarSlotChanged.AddDynamic(this, &ThisClass::OnHotbarSlotChanged);
	}
	EquipmentComponent = UInv_InventoryStatics::GetEquipmentComponent(OwningPlayerController.Get());

	SyncHotbarWithExistingInventory();
}

void UInv_HotbarComponent::RefreshHotbarFromInventory()
{
	SyncHotbarWithExistingInventory();
}

void UInv_HotbarComponent::OnHotbarSlotChanged(UInv_InventoryItem* Item, int32 Index)
{
	// This delegate can fire on the server (InventoryComponent RPC) or on clients.
	// If we are already on authority, apply the change directly; otherwise route via server RPC.
	if (AActor* Owner = GetOwner(); Owner && Owner->HasAuthority())
	{
		ApplyHotbarSlotChange(Item, Index);
	}
	else
	{
		Server_SetHotbarSlot(Item, Index);
	}
}

void UInv_HotbarComponent::ApplyHotbarSlotChange(UInv_InventoryItem* Item, int32 Index)
{
	if (Index >= MaxHotbarSlots || Index < 0) return;

	const bool bWasHotbarCompletelyEmpty = [this]()
	{
		for (int32 i = 0; i < MaxHotbarSlots; ++i)
		{
			if (HotbarSlots[i].bOccupied)
			{
				return false;
			}
		}
		return true;
	}();

	if (HotbarSlots[Index].bOccupied)	// clear the slot[Index]
	{
		UE_LOG(LogInventory, Warning, TEXT("clear the target slot %d, bOccupied = %d"),
											Index, HotbarSlots[Index].bOccupied);
		RemoveItemFromHotbar(Index);
	}

	if (!Item) return;	// return for nullptr to handle the RemoveItemFromHotbar case
	
	for (int32 i = 0; i < MaxHotbarSlots; i++)	// AddItemToHotbar case: 

	{
		UE_LOG(LogInventory, Warning, TEXT("slot %d, bOccupied = %d"), i, HotbarSlots[i].bOccupied);

		if (HotbarSlots[i].bOccupied && HotbarSlots[i].Item.Get() == Item)	// remove Item first if it's already in HotbarSlots

		{
			UE_LOG(LogInventory, Warning, TEXT("remove Item if it's already in slot[%d]"), i);
			RemoveItemFromHotbar(i);
			break;
		}
	}
	AddItemToHotbar(Item, Index);

	// First item onto an empty hotbar: mirror selecting that slot (equip if the item has equipment, otherwise no-op).
	if (Item && bWasHotbarCompletelyEmpty && InventoryComponent.IsValid() && EquipmentComponent.IsValid())
	{
		UInv_InventoryItem* const CurrentEquipped = EquipmentComponent->GetEquippedItem();
		InventoryComponent->RequestEquipItem(Item, CurrentEquipped);
	}
}

void UInv_HotbarComponent::Server_SetHotbarSlot_Implementation(UInv_InventoryItem* Item, int32 Index)
{
	ApplyHotbarSlotChange(Item, Index);
}

void UInv_HotbarComponent::ToggleEquipItem(int32 Index)
{
	if (!OwningPlayerController->IsLocalController()) return;
	if (!InventoryComponent.IsValid()) return;
	if (Index >= MaxHotbarSlots || Index < 0) return;
	if (!HotbarSlots[Index].bOccupied) return;

	// logs
	UE_LOG(LogInventory, Warning, TEXT("toggle equip at: %d"), Index);
	if (OwningPlayerController.IsValid())
	{
		FString OwnerName = OwningPlayerController->GetName();
		UE_LOG(LogTemp, Warning, TEXT("Hotbar Component owner's programmatic name: %s"), *OwnerName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("The Hotbar component has no owner!"));
	}
	Server_ToggleEquipItem(Index);
}

void UInv_HotbarComponent::Server_ToggleEquipItem_Implementation(int32 Index)
{
	if (!InventoryComponent.IsValid()) return;
	if (!EquipmentComponent.IsValid()) return;
	if (Index >= MaxHotbarSlots || Index < 0) return;

	UInv_InventoryItem* CurrentEquippedItem = EquipmentComponent->GetEquippedItem();
	if (!HotbarSlots[Index].bOccupied)
	{
		// Scrolling to an empty slot should unequip whatever is currently equipped.
		if (IsValid(CurrentEquippedItem))
		{
			InventoryComponent->RequestEquipItem(nullptr, CurrentEquippedItem);
		}
		return;
	}

	UInv_InventoryItem* TargetItem = GetItemInHotbarSlot(Index).Get();
	if (!IsValid(TargetItem)) return;

	// Scrolling to the currently equipped slot should keep it equipped.
	if (CurrentEquippedItem == TargetItem)
	{
		return;
	}

	// Otherwise equip target and explicitly unequip the current one.
	InventoryComponent->RequestEquipItem(TargetItem, CurrentEquippedItem);
}

void UInv_HotbarComponent::Client_UpdateHotbarSlot_Implementation(int32 SlotIndex, UInv_InventoryItem* Item)
{
	OnHotbarUpdated.Broadcast(SlotIndex, Item);
}

void UInv_HotbarComponent::SyncHotbarWithExistingInventory()
{
	if (!InventoryComponent.IsValid())
	{
		return;
	}

	for (UInv_InventoryItem* Item : InventoryComponent->GetAllInventoryItems())
	{
		bool bHotbarHasAnyOccupied = false;
		for (int32 s = 0; s < MaxHotbarSlots; ++s)
		{
			if (HotbarSlots[s].bOccupied)
			{
				bHotbarHasAnyOccupied = true;
				break;
			}
		}
		if (!InvShouldAutoAssignItemToHotbar(Item, bHotbarHasAnyOccupied))
		{
			continue;
		}

		bool bAlreadyInHotbar = false;
		for (int32 i = 0; i < MaxHotbarSlots; ++i)
		{
			if (HotbarSlots[i].bOccupied && HotbarSlots[i].Item.Get() == Item)
			{
				bAlreadyInHotbar = true;
				break;
			}
		}
		if (bAlreadyInHotbar)
		{
			continue;
		}

		for (int32 i = 0; i < MaxHotbarSlots; ++i)
		{
			if (!HotbarSlots[i].bOccupied)
			{
				if (AActor* Owner = GetOwner(); Owner && Owner->HasAuthority())
				{
					ApplyHotbarSlotChange(Item, i);
				}
				else
				{
					Server_SetHotbarSlot(Item, i);
				}
				break;
			}
		}
	}
}

void UInv_HotbarComponent::OnItemAdded(UInv_InventoryItem* Item)
{
	bool bHotbarHasAnyOccupied = false;
	for (int32 s = 0; s < MaxHotbarSlots; ++s)
	{
		if (HotbarSlots[s].bOccupied)
		{
			bHotbarHasAnyOccupied = true;
			break;
		}
	}
	if (!InvShouldAutoAssignItemToHotbar(Item, bHotbarHasAnyOccupied))
	{
		return;
	}

	for (int32 i = 0; i < MaxHotbarSlots; ++i)
	{
		if (HotbarSlots[i].bOccupied && HotbarSlots[i].Item.Get() == Item)
		{
			return;
		}
	}

	for (int i = 0; i < MaxHotbarSlots; ++i)
	{
		if (!HotbarSlots[i].bOccupied)
		{
			if (AActor* Owner = GetOwner(); Owner && Owner->HasAuthority())
			{
				ApplyHotbarSlotChange(Item, i);
			}
			else
			{
				Server_SetHotbarSlot(Item, i);
			}
			break;
		}
	}
}

void UInv_HotbarComponent::OnItemRemoved(UInv_InventoryItem* Item)
{
	// Always clear slots referencing this item (do not gate on InvShouldAutoAssign — the slot may still show a ghost icon otherwise).
	if (!IsValid(Item))
	{
		return;
	}

	for (int i = 0; i < MaxHotbarSlots; ++i)
	{
		if (HotbarSlots[i].bOccupied && HotbarSlots[i].Item.Get() == Item)
		{
			Server_SetHotbarSlot(nullptr, i);
			break;
		}
	}
}

void UInv_HotbarComponent::AddItemToHotbar(TWeakObjectPtr<UInv_InventoryItem> Item, int32 Index)
{
	HotbarSlots[Index].Item = Item;
	HotbarSlots[Index].bOccupied = true;
	UE_LOG(LogInventory, Warning, TEXT("Add Item to slot %d, bOccupied = %d"),
											Index, HotbarSlots[Index].bOccupied);
	Client_UpdateHotbarSlot(Index, Item.Get());
}

void UInv_HotbarComponent::RemoveItemFromHotbar(int32 Index)
{
	// just for inventory UI update, -1 will be rejected by hotbar UI 
	Client_UpdateHotbarSlot(-1, GetItemInHotbarSlot(Index).Get());

	HotbarSlots[Index].Item = nullptr;
	HotbarSlots[Index].bOccupied = false;
	UE_LOG(LogInventory, Warning, TEXT("Remove Item from slot %d, bOccupied = %d"),
											Index, HotbarSlots[Index].bOccupied);
	Client_UpdateHotbarSlot(Index, nullptr);
}
