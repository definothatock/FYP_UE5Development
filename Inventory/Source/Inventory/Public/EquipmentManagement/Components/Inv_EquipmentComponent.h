// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory.h"
#include "Components/ActorComponent.h"
#include "Inv_EquipmentComponent.generated.h"

class UInv_HotbarComponent;
struct FGameplayTag;
struct FInv_ItemManifest;
struct FInv_EquipmentFragment;
class AInv_EquipActor;
class UInv_InventoryComponent;
class UInv_InventoryItem;
class APlayerController;
class USkeletalMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UInv_EquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	void SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh);
	void SetIsProxy(bool bProxy) { bIsProxy = bProxy; }
	void InitializeOwner(APlayerController* PlayerController);
	bool HasSpawnedEquippedActor() const;

	// int32 GetEquippedGridIndex() const
	// {
	// 	return EquippedGridIndex;
	// }
	// void SetEquippedGridIndex(int32 Index)
	// {
	// 	EquippedGridIndex = Index;
	// 	UE_LOG(LogInventory, Warning , TEXT("SetEquippedGridIndex = %d"), EquippedGridIndex);
	// }
	UInv_InventoryItem* GetEquippedItem() const
	{
		return Item.Get();
	}

	UFUNCTION(BlueprintPure, Category="Inventory")
	AInv_EquipActor* GetEquippedActorByIndex(int32 Index) const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	const TArray<AInv_EquipActor*>& GetAllEquippedActors() const;

protected:
	
	virtual void BeginPlay() override;

private:

	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<APlayerController> OwningPlayerController;
	TWeakObjectPtr<USkeletalMeshComponent> OwningSkeletalMesh;

	UFUNCTION()
	void OnItemEquipped(UInv_InventoryItem* EquippedItem);

	UFUNCTION()
	void OnItemUnequipped(UInv_InventoryItem* UnequippedItem);

	void InitPlayerController();
	void InitInventoryComponent();
	AInv_EquipActor* SpawnEquippedActor(FInv_EquipmentFragment* EquipmentFragment, const FInv_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh);
	
	UPROPERTY()
	TArray<TObjectPtr<AInv_EquipActor>> EquippedActors;
	
	UPROPERTY()
	TWeakObjectPtr<UInv_InventoryItem> Item;
	
	// int32 EquippedGridIndex = -1;

	AInv_EquipActor* FindEquippedActor(const FGameplayTag& EquipmentTypeTag);
	void RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag);

	UFUNCTION()
	void OnPossessedPawnChange(APawn* OldPawn, APawn* NewPawn);

	bool bIsProxy{false};
};
