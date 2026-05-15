// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Inv_EquipActor.generated.h"

UCLASS()
class INVENTORY_API AInv_EquipActor : public AActor
{
	GENERATED_BODY()

public:
	AInv_EquipActor();

	FGameplayTag GetEquipmentType() const { return EquipmentType; }
	void SetEquipmentType(FGameplayTag Type) { EquipmentType = Type; }

	void SetOwningPawn(APawn* OwningPawn) { OwningPlayerPawn = OwningPawn; }
	void SetOwningPlayer(AActor* OwningOwner) { Owner = OwningOwner; }

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TWeakObjectPtr<APlayerController> OwningPlayerController;
	
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TWeakObjectPtr<APawn> OwningPlayerPawn;
	
private:
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FGameplayTag EquipmentType;
};
