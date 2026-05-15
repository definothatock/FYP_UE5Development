// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Inv_SpatialInventory.generated.h"

struct FGameplayTag;
class UInv_EquippedGridSlot;
class UInv_ItemDescription;
class UInv_InventoryGrid;
class UWidgetSwitcher;
class UButton;
class UCanvasPanel;
class UInv_HoverItem;
class UInv_InventoryComponent;
class UInv_EquipmentComponent;


UCLASS()
class INVENTORY_API UInv_SpatialInventory : public UInv_InventoryBase
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FInv_SlotAvailabilityResult HasRoomForItem(UInv_ItemComponent* ItemComponent) const override;
	virtual void OnItemHovered(UInv_InventoryItem* Item) override;
	virtual void OnItemUnHovered() override;
	virtual bool HasHoverItem() const override;
	virtual UInv_HoverItem* GetHoverItem() const override;
	virtual float GetTileSize() const override;
private:

	// UPROPERTY()
	// TArray<TObjectPtr<UInv_EquippedGridSlot>> EquippedGridSlots;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CanvasPanel;

	// UPROPERTY(meta = (BindWidget))
	// TObjectPtr<UWidgetSwitcher> Switcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInv_InventoryGrid> Grid;
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_ItemDescription> ItemDescriptionClass;

	UPROPERTY()
	TObjectPtr<UInv_ItemDescription> ItemDescription;

	// UPROPERTY(EditAnywhere, Category = "Inventory")
	// TSubclassOf<UInv_ItemDescription> EquippedItemDescriptionClass;

	// UPROPERTY()
	// TObjectPtr<UInv_ItemDescription> EquippedItemDescription;

	FTimerHandle DescriptionTimer;
	// FTimerHandle EquippedDescriptionTimer;

	// UFUNCTION()
	// void ShowEquippedItemDescription(UInv_InventoryItem* Item);

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float DescriptionTimerDelay = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	float EquippedDescriptionTimerDelay = 0.5f;

	UInv_ItemDescription* GetItemDescription();
	// UInv_ItemDescription* GetEquippedItemDescription();
	
	UFUNCTION()
	void ShowGrid();
	void SetActiveGrid(UInv_InventoryGrid* Grid);

	///////////
	UFUNCTION()
	void RequestEquipItem_SlotClicked(UInv_GridSlot* GridSlot, const FGameplayTag& EquipmentTypeTag);
	
	UFUNCTION()
	void TryAddHotbarSlot(UInv_InventoryItem* Item, int32 HotbarIndex);
	//////////
	
	void SetItemDescriptionSizeAndPosition(UInv_ItemDescription* Description, UCanvasPanel* Canvas) const;
	// void SetEquippedItemDescriptionSizeAndPosition(UInv_ItemDescription* Description, UInv_ItemDescription* EquippedDescription, UCanvasPanel* Canvas) const;
	// bool CanEquipHoverItem(UInv_EquippedGridSlot* EquippedGridSlot, const FGameplayTag& EquipmentTypeTag) const;
	// bool CanEquipHoverItem(UInv_GridSlot* GridSlot, const FGameplayTag& EquipmentTypeTag) const;
	// UInv_EquippedGridSlot* FindSlotWithEquippedItem(UInv_InventoryItem* EquippedItem) const;
	// void ClearSlotOfItem(UInv_EquippedGridSlot* EquippedGridSlot);
	void BroadcastRequestEquipItemDelegates(UInv_GridSlot* GridSlot, UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip) const;
	
	TWeakObjectPtr<UInv_InventoryGrid> ActiveGrid;
	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<UInv_EquipmentComponent> EquipmentComponent;
};
