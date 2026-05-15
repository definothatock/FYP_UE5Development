// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inv_ItemPopUp.generated.h"

/**
 * The item popup widget shows up when right-clicking on an item
 * in the inventory grid.
 */
class UButton;
class USlider;
class UTextBlock;
class USizeBox;

DECLARE_DYNAMIC_DELEGATE_OneParam(FPopUpMenuEquip, int32, Index);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FPopUpMenuSplit, int32, SplitAmount, int32, Index);
DECLARE_DYNAMIC_DELEGATE_OneParam(FPopUpMenuDrop, int32, Index);
DECLARE_DYNAMIC_DELEGATE_OneParam(FPopUpMenuConsume, int32, Index);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FPopUpMenuLogToHotbar, int32, HotbarIndex, int32, GridIndex);


UCLASS()
class INVENTORY_API UInv_ItemPopUp : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	FPopUpMenuEquip OnEquip;
	FPopUpMenuSplit OnSplit;
	FPopUpMenuDrop OnDrop;
	FPopUpMenuConsume OnConsume;
	FPopUpMenuLogToHotbar OnLogToHotbar;
	
	int32 GetSplitAmount() const;
	void CollapseEquipRelatedButtons() const;
	void CollapseSplitButton() const;
	void CollapseConsumeButton() const;
	void SetSliderParams(const float Max, const float Value) const;
	FVector2D GetBoxSize() const;
	void SetGridIndex(int32 Index) { GridIndex = Index; }
	int32 GetGridIndex() const { return GridIndex; }
	
private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Equip;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Split;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Drop;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Consume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Split;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SplitAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox_Root;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_LogToHotbar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_NumKey1;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_NumKey2;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_NumKey3;
	
	int32 GridIndex{INDEX_NONE};

	UFUNCTION()
	void EquipButtonClicked();
	
	UFUNCTION()
	void ConsumeButtonClicked();	

	UFUNCTION()
	void SplitButtonClicked();
	UFUNCTION()
	void SliderValueChanged(float Value);
	
	UFUNCTION()
	void DropButtonClicked();

	UFUNCTION()
	void Button_NumKey1Clicked();
	
	UFUNCTION()
	void Button_NumKey2Clicked();
	
	UFUNCTION()
	void Button_NumKey3Clicked();
};
