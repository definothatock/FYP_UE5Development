#pragma once

#include "CoreMinimal.h"
#include "Inventory.h"
#include "Components/ActorComponent.h"
#include "Inv_HotbarComponent.generated.h"

struct FGameplayTag;
struct FInv_ItemManifest;
struct FInv_HotbarSlot;
class UInv_EquipmentComponent;
class UInv_InventoryComponent;
class UInv_InventoryItem;
class APlayerController;

USTRUCT(BlueprintType)
struct FInv_HotbarSlot
{
	GENERATED_BODY()
    
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UInv_InventoryItem> Item = nullptr;

	UPROPERTY(BlueprintReadOnly)
	bool bOccupied = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FHotbarStatusChanged,
	int32, SlotIndex,
	UInv_InventoryItem*, Item
);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UInv_HotbarComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UInv_HotbarComponent();

    UPROPERTY(EditDefaultsOnly)
    int32 MaxHotbarSlots = 3;
    
    UPROPERTY(Replicated)
    TArray<FInv_HotbarSlot> HotbarSlots;

	virtual void BeginPlay() override;
	void InitializeOwner(APlayerController* PlayerController);

	UFUNCTION()
	TWeakObjectPtr<UInv_InventoryItem> GetItemInHotbarSlot(int32 Index) {return HotbarSlots[Index].Item;};

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetIsOccupiedInHotbarSlot(int32 Index)
	{
	if (!HotbarSlots.IsValidIndex(Index))
		{
			UE_LOG(LogInventory, Warning,
				TEXT("GetIsOccupiedInHotbarSlot invalid index %d (Num=%d)"),
				Index, HotbarSlots.Num());
			return false;
		}

		return HotbarSlots[Index].bOccupied;
	};

	bool IsValidSlotIndex(int32 Index) const
	{
		return HotbarSlots.IsValidIndex(Index);
	}

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_SetHotbarSlot(UInv_InventoryItem* Item, int32 Index);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleEquipItem(int32 Index);
	
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_ToggleEquipItem(int32 Index);

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Inventory")
	void Client_UpdateHotbarSlot(int32 SlotIndex, UInv_InventoryItem* Item);

	/** Re-scan inventory and fill empty hotbar slots (Blueprint / manual sync). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshHotbarFromInventory();
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Inventory")
	FHotbarStatusChanged OnHotbarUpdated;
	
private:
	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<UInv_EquipmentComponent> EquipmentComponent;
	TWeakObjectPtr<APlayerController> OwningPlayerController;

	void InitInventoryComponent();

	UFUNCTION()
	void OnHotbarSlotChanged(UInv_InventoryItem* Item, int32 Index);
	
	UFUNCTION()
	void OnItemAdded(UInv_InventoryItem* Item);
	
	UFUNCTION()
	void OnItemRemoved(UInv_InventoryItem* Item);

	void AddItemToHotbar(TWeakObjectPtr<UInv_InventoryItem> Item, int32 Index);
	void RemoveItemFromHotbar(int32 Index);

	/** Server-side slot update shared by Server_SetHotbarSlot and native refresh (avoids RPC edge cases on dedicated server). */
	void ApplyHotbarSlotChange(UInv_InventoryItem* Item, int32 Index);

	/** Items added before the hotbar subscribed to OnItemAdded are easy to miss; re-sync once after bind. */
	void SyncHotbarWithExistingInventory();
};


