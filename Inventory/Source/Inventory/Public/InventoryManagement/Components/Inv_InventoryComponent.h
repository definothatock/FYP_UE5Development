// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryManagement/FastArray/Inv_FastArray.h"
#include "Items/Manifest/Inv_ItemManifest.h"
#include "Inv_InventoryComponent.generated.h"

class UInv_ItemComponent;
class UInv_InventoryItem;
class UInv_InventoryBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryItemChange, UInv_InventoryItem*, Item);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNoRoomInInventory);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStackChange, const FInv_SlotAvailabilityResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryMenuToggled, bool, bOpen);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemEquipStatusChanged, UInv_InventoryItem*, Item);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHotbarSlotChanged, UInv_InventoryItem*, Item, int32, HotbarSlotIndex);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UInv_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UInv_InventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void TryAddItem(UInv_ItemComponent* ItemComponent);

	/** Server only: add one item from manifest without a world pickup. Does not run spatial grid room checks; use when inventory has space.
	 *  Does not equip and does not auto-assign to the hotbar (not treated as pickup). Not BlueprintAuthorityOnly: listen-server host PC can fail HasAuthority(); native code enforces server-side mutation. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AuthorityGrantItemFromManifest(const FInv_ItemManifest& Manifest);

	UFUNCTION(Server, Reliable)
	void Server_AddNewItem(UInv_ItemComponent* ItemComponent, int32 StackCount, int32 Remainder);

	UFUNCTION(Server, Reliable)
	void Server_AddStacksToItem(UInv_ItemComponent* ItemComponent, int32 StackCount, int32 Remainder);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_DropItem(UInv_InventoryItem* Item, int32 StackCount);

	/** Remove stacks from inventory and unequip using the spawned equip actor (e.g. Self). No world pickup — item is destroyed/discarded. StackCount <= 0 removes the full stack. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_DropItemFromEquipActor(AActor* EquipActor, int32 StackCount);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_ConsumeItem(UInv_InventoryItem* Item);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_ClearInventoryAndSaveScore();

	UFUNCTION(Client, Reliable)
	void Client_PrintClearedScore(int32 ClearedScore);
	
	//////////
	void RequestEquipItem(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip);

	UFUNCTION(Server, Reliable)
	void Server_RequestEquipItem(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_RequestEquipItem(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip);

	void SetHotbarSlot(UInv_InventoryItem* Item, int32 SlotIndex);
	
	UFUNCTION(Server, Reliable)
	void Server_SetHotbarSlot(UInv_InventoryItem* Item, int32 SlotIndex);
	//////////
	
	void ToggleInventoryMenu();
	void AddRepSubObj(UObject* SubObj);
	void SpawnDroppedItem(UInv_InventoryItem* Item, int32 StackCount);
	UInv_InventoryBase* GetInventoryMenu() const { return InventoryMenu; }
	bool IsMenuOpen() const { return bInventoryMenuOpen; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Score")
	int32 GetLastClearedScore() const { return LastClearedScore; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Score")
	int32 GetAccumulatedClearedScore() const { return AccumulatedClearedScore; }

	/** All items currently in the replicated inventory list (e.g. for hotbar sync after startup grants). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<UInv_InventoryItem*> GetAllInventoryItems() const;

	FInventoryItemChange OnItemAdded;
	FInventoryItemChange OnItemRemoved;
	
	FInventoryMenuToggled OnInventoryMenuToggled;
	FNoRoomInInventory NoRoomInInventory;
	FStackChange OnStackChange;
	
	FItemEquipStatusChanged OnItemEquipped;
	FItemEquipStatusChanged OnItemUnequipped;

	FHotbarSlotChanged OnHotbarSlotChanged;
	
protected:
	
	virtual void BeginPlay() override;

private:

	TWeakObjectPtr<APlayerController> OwningController;

	/** Subtracts stacks (or removes entry) and broadcasts unequip; does not spawn a pickup. */
	void RemoveInventoryStacksAndUnequip(UInv_InventoryItem* Item, int32 StackCount);
	
	void ConstructInventory();

	UPROPERTY(Replicated)
	FInv_InventoryFastArray InventoryList;

	UPROPERTY(Replicated)
	int32 LastClearedScore = 0;

	UPROPERTY(Replicated)
	int32 AccumulatedClearedScore = 0;

	UPROPERTY()
	TObjectPtr<UInv_InventoryBase> InventoryMenu;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_InventoryBase> InventoryMenuClass;

	bool bInventoryMenuOpen;
	void OpenInventoryMenu();
	void CloseInventoryMenu();

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float DropSpawnAngleMin = -85.f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float DropSpawnAngleMax = 85.f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float DropSpawnDistanceMin = 10.f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float DropSpawnDistanceMax = 50.f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float RelativeSpawnElevation = 70.f;
};
