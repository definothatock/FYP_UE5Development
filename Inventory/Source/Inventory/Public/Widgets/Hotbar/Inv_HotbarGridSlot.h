#pragma once

#include "CoreMinimal.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "HotbarManagement/Inv_HotbarComponent.h"
#include "Inv_HotbarGridSlot.generated.h"

class UInv_InventoryItem;
class UInv_InventoryComponent;
class UInv_EquippedSlottedItem;
class UInv_SlottedItem;

UCLASS()
class INVENTORY_API UInv_HotbarGridSlot : public UUserWidget
{
	GENERATED_BODY()
public:
	// void OnItemEquipped(UInv_InventoryItem* Item, const FGameplayTag& EquipmentTag, float TileSize);
	// void Selected();

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UFUNCTION(BlueprintCallable, Category= "Inventory")
	void InitializeHotbarSlot(UInv_InventoryComponent* IC, UInv_HotbarComponent* HC);
	
	UFUNCTION(BlueprintCallable, Category= "Inventory")
	void UpdateHotbarSlot(int32 BroadcastSlotIndex, UInv_InventoryItem* Item);

	UFUNCTION(BlueprintCallable, Category= "Inventory")
	void SetHotbarSlotIndex(int32 Index) { SlotIndex = Index; TXT_NumKey->SetText(FText::AsNumber(Index+1));};

	UPROPERTY(meta = (BindWidget), BlueprintReadWrite)
	TObjectPtr<UBorder> BRD_Border;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	bool bSelected = false;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_NumKey;
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<UInv_SlottedItem> SlottedItemClass;

	int32 SlotIndex;
	
	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;

	TWeakObjectPtr<UInv_HotbarComponent> HotbarComponent;
	
	void CreateImage(const UInv_InventoryItem* Item) const;
};
