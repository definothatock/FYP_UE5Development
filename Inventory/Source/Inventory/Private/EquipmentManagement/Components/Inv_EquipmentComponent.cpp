// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentManagement/Components/Inv_EquipmentComponent.h"

#include "EquipmentManagement/EquipActor/Inv_EquipActor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "InventoryManagement/Utils/Inv_InventoryStatics.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Fragments/Inv_ItemFragment.h"


void UInv_EquipmentComponent::SetOwningSkeletalMesh(USkeletalMeshComponent* OwningMesh)
{
	OwningSkeletalMesh = OwningMesh;
}

void UInv_EquipmentComponent::InitializeOwner(APlayerController* PlayerController)
{
	if (IsValid(PlayerController))
	{
		OwningPlayerController = PlayerController;
	}
	InitInventoryComponent();
}

bool UInv_EquipmentComponent::HasSpawnedEquippedActor() const
{
	for (const AInv_EquipActor* EquippedActor : EquippedActors)
	{
		if (IsValid(EquippedActor))
		{
			return true;
		}
	}
	return false;
}

void UInv_EquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	
	InitPlayerController();
}

AInv_EquipActor* UInv_EquipmentComponent::GetEquippedActorByIndex(int32 Index) const
{
	return EquippedActors.IsValidIndex(Index) ? EquippedActors[Index] : nullptr;
}

const TArray<AInv_EquipActor*>& UInv_EquipmentComponent::GetAllEquippedActors() const
{
	return EquippedActors;
}

void UInv_EquipmentComponent::InitPlayerController()
{
	if (OwningPlayerController = Cast<APlayerController>(GetOwner()); OwningPlayerController.IsValid())
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(OwningPlayerController->GetPawn()); IsValid(OwnerCharacter))
		{
			OnPossessedPawnChange(nullptr, OwnerCharacter);
		}
		else
		{
			OwningPlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChange);
		}
	}
}

void UInv_EquipmentComponent::OnPossessedPawnChange(APawn* OldPawn, APawn* NewPawn)
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(NewPawn); IsValid(OwnerCharacter))
	{
		OwningSkeletalMesh = OwnerCharacter->GetMesh();
	}
	InitInventoryComponent();
}

void UInv_EquipmentComponent::InitInventoryComponent()
{
	InventoryComponent = UInv_InventoryStatics::GetInventoryComponent(OwningPlayerController.Get());
	if (!InventoryComponent.IsValid()) return;

	if (!InventoryComponent->OnItemEquipped.IsAlreadyBound(this, &ThisClass::OnItemEquipped))
	{
		InventoryComponent->OnItemEquipped.AddDynamic(this, &ThisClass::OnItemEquipped);
	}

	if (!InventoryComponent->OnItemUnequipped.IsAlreadyBound(this, &ThisClass::OnItemUnequipped))
	{
		InventoryComponent->OnItemUnequipped.AddDynamic(this, &ThisClass::OnItemUnequipped);
	}
}

AInv_EquipActor* UInv_EquipmentComponent::SpawnEquippedActor(FInv_EquipmentFragment* EquipmentFragment, const FInv_ItemManifest& Manifest, USkeletalMeshComponent* AttachMesh)
{
	AInv_EquipActor* SpawnedEquipActor = EquipmentFragment->SpawnAttachedActor(AttachMesh);
	SpawnedEquipActor->SetEquipmentType(EquipmentFragment->GetEquipmentType());
	SpawnedEquipActor->SetOwner(GetOwner());
	SpawnedEquipActor->SetOwningPlayer(GetOwner());
	SpawnedEquipActor-> SetOwningPawn(Cast<APlayerController>(GetOwner())->GetPawn());
	EquipmentFragment->SetEquippedActor(SpawnedEquipActor);
	return SpawnedEquipActor;
}

AInv_EquipActor* UInv_EquipmentComponent::FindEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	auto FoundActor = EquippedActors.FindByPredicate([&EquipmentTypeTag](const AInv_EquipActor* EquippedActor)
	{
		return EquippedActor->GetEquipmentType().MatchesTagExact(EquipmentTypeTag);
	});
	return FoundActor ? *FoundActor : nullptr;
}

void UInv_EquipmentComponent::RemoveEquippedActor(const FGameplayTag& EquipmentTypeTag)
{
	/*if (AInv_EquipActor* EquippedActor = FindEquippedActor(EquipmentTypeTag); IsValid(EquippedActor))
	{
v		EquippedActor->Destroy();
	}*/
	AInv_EquipActor* EquippedActor = EquippedActors[0];
	EquippedActors[0]->Destroy();
	EquippedActors.Empty();
	EquippedActor->Destroy();
}

void UInv_EquipmentComponent::OnItemEquipped(UInv_InventoryItem* EquippedItem)
{
	if (!IsValid(EquippedItem)) return;
	// Log caller context and authority state
	AActor* OwnerActor = GetOwner();
	const bool bHasAuthority = OwningPlayerController.IsValid() ? OwningPlayerController->HasAuthority() : false;
	UE_LOG(LogInventory, Warning, TEXT("OnItemEquipped called on ComponentOwner=%s, OwningPlayerControllerHasAuthority=%d, EquippedItem=%p (%s)"),
		OwnerActor ? *OwnerActor->GetName() : TEXT("null"), bHasAuthority,
		EquippedItem, *EquippedItem->GetName());

	if (!OwningPlayerController->HasAuthority()) return;

	FInv_ItemManifest& ItemManifest = EquippedItem->GetItemManifestMutable();
	FInv_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FInv_EquipmentFragment>();
	if (!EquipmentFragment) return;

	UE_LOG(LogInventory, Warning, TEXT("EquipmentFragment for item %s: bEquipped=%d, bIsProxy=%d"),
		*EquippedItem->GetName(), EquipmentFragment->bEquipped, bIsProxy);

	if (!bIsProxy)
	{
		EquipmentFragment->OnEquip(OwningPlayerController.Get());
	}

	if (!OwningSkeletalMesh.IsValid()) return;
	AInv_EquipActor* SpawnedEquipActor = SpawnEquippedActor(EquipmentFragment, ItemManifest, OwningSkeletalMesh.Get());

	// also handle unequip if there's already equipped item
	// (bypassed the original clicked equipped slotted item)
	if (EquippedActors.Num() > 0)
		RemoveEquippedActor(EquipmentFragment->GetEquipmentType());

	this->Item = EquippedItem;
	EquippedActors.Add(SpawnedEquipActor);
	UE_LOG(LogInventory, Warning, TEXT("equip ac done equipped"));
}

void UInv_EquipmentComponent::OnItemUnequipped(UInv_InventoryItem* UnequippedItem)
{
	if (!IsValid(UnequippedItem)) return;
	// Log caller context and authority state
	AActor* OwnerActor = GetOwner();
	const bool bHasAuthority = OwningPlayerController.IsValid() ? OwningPlayerController->HasAuthority() : false;
	UE_LOG(LogInventory, Warning, TEXT("OnItemUnequipped called on ComponentOwner=%s, OwningPlayerControllerHasAuthority=%d, UnequippedItem=%p (%s)"),
		OwnerActor ? *OwnerActor->GetName() : TEXT("null"), bHasAuthority,
		UnequippedItem, *UnequippedItem->GetName());

	if (!OwningPlayerController->HasAuthority()) return;

	// identity check
	if (this->Item != UnequippedItem)
	{
		return; // not the equipped item → ignore
	}
	
	FInv_ItemManifest& ItemManifest = UnequippedItem->GetItemManifestMutable();
	FInv_EquipmentFragment* EquipmentFragment = ItemManifest.GetFragmentOfTypeMutable<FInv_EquipmentFragment>();
	if (!EquipmentFragment) return;

	UE_LOG(LogInventory, Warning, TEXT("EquipmentFragment for item %s: bEquipped=%d, bIsProxy=%d"),
		*UnequippedItem->GetName(), EquipmentFragment->bEquipped, bIsProxy);

	if (!bIsProxy)
	{
		EquipmentFragment->OnUnequip(OwningPlayerController.Get());
	}

	this->Item = nullptr;
	UE_LOG(LogTemp, Warning, TEXT("OnItemUnequipped"));
	RemoveEquippedActor(EquipmentFragment->GetEquipmentType());
}


