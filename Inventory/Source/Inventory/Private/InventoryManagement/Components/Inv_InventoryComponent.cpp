// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Inv_InventoryComponent.h"

#include "Inventory.h"
#include "EquipmentManagement/Components/Inv_EquipmentComponent.h"
#include "EquipmentManagement/EquipActor/Inv_EquipActor.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Widgets/Inventory/InventoryBase/Inv_InventoryBase.h"
#include "Net/UnrealNetwork.h"
#include "Items/Inv_InventoryItem.h"
#include "Items/Fragments/Inv_ItemFragment.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/Inv_InventoryScoreSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"


UInv_InventoryComponent::UInv_InventoryComponent() : InventoryList(this)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	bInventoryMenuOpen = false;
}

void UInv_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
	DOREPLIFETIME(ThisClass, LastClearedScore);
	DOREPLIFETIME(ThisClass, AccumulatedClearedScore);
}

void UInv_InventoryComponent::AuthorityGrantItemFromManifest(const FInv_ItemManifest& Manifest)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	// Listen-server host: PlayerController can fail HasAuthority() in some setups while GameMode logic still runs on the server.
	const ENetMode NetMode = World->GetNetMode();
	const bool bCanMutateInventory =
		OwnerActor->HasAuthority()
		|| (Cast<APlayerController>(OwnerActor) && (NetMode == NM_ListenServer || NetMode == NM_Standalone));
	if (!bCanMutateInventory)
	{
		return;
	}

	UInv_InventoryItem* NewItem = InventoryList.AddEntryFromManifest(Manifest);
	if (!IsValid(NewItem))
	{
		return;
	}

	// Not a world pickup: do not auto-assign to hotbar (pickups use the same OnItemAdded path with this flag false).
	NewItem->SetExcludeFromHotbarAutoAssign(true);

	// Always notify listeners on authority (including dedicated server). Clients also receive PostReplicatedAdd → OnItemAdded.
	OnItemAdded.Broadcast(NewItem);
}

void UInv_InventoryComponent::TryAddItem(UInv_ItemComponent* ItemComponent)
{
	FInv_SlotAvailabilityResult Result = InventoryMenu->HasRoomForItem(ItemComponent);

	UInv_InventoryItem* FoundItem = InventoryList.FindFirstItemByType(ItemComponent->GetItemManifest().GetItemType());
	Result.Item = FoundItem;
	
	if (Result.TotalRoomToFill == 0)
	{
		NoRoomInInventory.Broadcast();
		return;
	}
	
	if (Result.Item.IsValid() && Result.bStackable)
	{
		// Add stacks to an item that already exists in the inventory. We only want to update the stack count,
		// not create a new item of this type.
		OnStackChange.Broadcast(Result);
		Server_AddStacksToItem(ItemComponent, Result.TotalRoomToFill, Result.Remainder);
	}
	else if (Result.TotalRoomToFill > 0)
	{
		// This item type doesn't exist in the inventory. Create a new one and update all pertinent slots.
		Server_AddNewItem(ItemComponent, Result.bStackable ? Result.TotalRoomToFill : 0, Result.Remainder);
	}
}

void UInv_InventoryComponent::Server_AddNewItem_Implementation(UInv_ItemComponent* ItemComponent, int32 StackCount, int32 Remainder)
{
	UInv_InventoryItem* NewItem = InventoryList.AddEntry(ItemComponent);
	NewItem->SetTotalStackCount(StackCount);

	if (GetOwner()->GetNetMode() == NM_ListenServer || GetOwner()->GetNetMode() == NM_Standalone)
	{
		OnItemAdded.Broadcast(NewItem);
	}

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
	}
	else if (FInv_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
}

void UInv_InventoryComponent::Server_AddStacksToItem_Implementation(UInv_ItemComponent* ItemComponent, int32 StackCount, int32 Remainder)
{
	const FGameplayTag& ItemType = IsValid(ItemComponent) ? ItemComponent->GetItemManifest().GetItemType() : FGameplayTag::EmptyTag;
	UInv_InventoryItem* Item = InventoryList.FindFirstItemByType(ItemType);
	if (!IsValid(Item)) return;

	Item->SetTotalStackCount(Item->GetTotalStackCount() + StackCount);

	if (Remainder == 0)
	{
		ItemComponent->PickedUp();
	}
	else if (FInv_StackableFragment* StackableFragment = ItemComponent->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(Remainder);
	}
}

void UInv_InventoryComponent::RemoveInventoryStacksAndUnequip(UInv_InventoryItem* Item, int32 StackCount)
{
	if (!IsValid(Item))
	{
		return;
	}

	const int32 NewStackCount = Item->GetTotalStackCount() - StackCount;
	if (NewStackCount <= 0)
	{
		// Authority: broadcast so hotbar/UI update on the server too (not only listen/standalone).
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			OnItemRemoved.Broadcast(Item);
		}
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}

	Server_RequestEquipItem(nullptr, Item);
}

void UInv_InventoryComponent::Server_DropItem_Implementation(UInv_InventoryItem* Item, int32 StackCount)
{
	RemoveInventoryStacksAndUnequip(Item, StackCount);
	SpawnDroppedItem(Item, StackCount);
}

void UInv_InventoryComponent::Server_DropItemFromEquipActor_Implementation(AActor* EquipActor, int32 StackCount)
{
	if (!IsValid(EquipActor))
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!IsValid(PC))
	{
		return;
	}

	UInv_EquipmentComponent* EquipComp = PC->FindComponentByClass<UInv_EquipmentComponent>();
	if (!IsValid(EquipComp))
	{
		return;
	}

	bool bActorIsEquipped = false;
	for (const AInv_EquipActor* Spawned : EquipComp->GetAllEquippedActors())
	{
		if (Spawned == EquipActor)
		{
			bActorIsEquipped = true;
			break;
		}
	}
	if (!bActorIsEquipped)
	{
		return;
	}

	UInv_InventoryItem* Item = EquipComp->GetEquippedItem();
	if (!IsValid(Item))
	{
		return;
	}

	if (StackCount <= 0)
	{
		StackCount = Item->GetTotalStackCount();
	}

	RemoveInventoryStacksAndUnequip(Item, StackCount);
}

void UInv_InventoryComponent::SpawnDroppedItem(UInv_InventoryItem* Item, int32 StackCount)
{
	const APawn* OwningPawn = OwningController->GetPawn();
	if (!IsValid(OwningPawn)) return;

	FVector RotatedForward = OwningPawn->GetActorForwardVector();
	RotatedForward = RotatedForward.RotateAngleAxis(FMath::FRandRange(DropSpawnAngleMin, DropSpawnAngleMax), FVector::UpVector);
	RotatedForward.Z = 0.f;
	RotatedForward = RotatedForward.GetSafeNormal();
	const float MinForwardDistance = FMath::Max(DropSpawnDistanceMin, OwningPawn->GetSimpleCollisionRadius() + 30.f);
	const float MaxForwardDistance = FMath::Max(DropSpawnDistanceMax, MinForwardDistance);
	FVector SpawnLocation = OwningPawn->GetActorLocation() + RotatedForward * FMath::FRandRange(MinForwardDistance, MaxForwardDistance);
	SpawnLocation.Z += RelativeSpawnElevation;
	const FRotator SpawnRotation = FRotator::ZeroRotator;

	FInv_ItemManifest& ItemManifest = Item->GetItemManifestMutable();
	if (FInv_StackableFragment* StackableFragment = ItemManifest.GetFragmentOfTypeMutable<FInv_StackableFragment>())
	{
		StackableFragment->SetStackCount(StackCount);
	}
	ItemManifest.SpawnPickupActor(this, SpawnLocation, SpawnRotation, ItemManifest);
}

void UInv_InventoryComponent::Server_ConsumeItem_Implementation(UInv_InventoryItem* Item)
{
	const int32 NewStackCount = Item->GetTotalStackCount() - 1;
	if (NewStackCount <= 0)
	{
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			OnItemRemoved.Broadcast(Item);
		}
		InventoryList.RemoveEntry(Item);
	}
	else
	{
		Item->SetTotalStackCount(NewStackCount);
	}

	if (FInv_ConsumableFragment* ConsumableFragment = Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_ConsumableFragment>())
	{
		ConsumableFragment->OnConsume(OwningController.Get());
	}
}

void UInv_InventoryComponent::Server_ClearInventoryAndSaveScore_Implementation()
{
	const TArray<UInv_InventoryItem*> Items = InventoryList.GetAllItems();
	int32 ClearedScore = 0;

	for (UInv_InventoryItem* Item : Items)
	{
		if (!IsValid(Item)) continue;

		const int32 UnitScore = Item->GetItemScore();
		const int32 StackCount = FMath::Max(1, Item->GetTotalStackCount());
		ClearedScore += UnitScore * StackCount;

		// Ensure equipment listeners reset any currently equipped item state.
		Server_RequestEquipItem(nullptr, Item);

		if (GetOwner() && GetOwner()->HasAuthority())
		{
			OnItemRemoved.Broadcast(Item);
		}
		InventoryList.RemoveEntry(Item);
	}

	LastClearedScore = ClearedScore;
	AccumulatedClearedScore += ClearedScore;

	if (OwningController.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UInv_InventoryScoreSubsystem* ScoreSubsystem = GameInstance->GetSubsystem<UInv_InventoryScoreSubsystem>())
				{
					ScoreSubsystem->BroadcastClearedScoreCommitted(OwningController.Get(), ClearedScore, AccumulatedClearedScore);
				}
			}
		}
	}

	Client_PrintClearedScore(ClearedScore);
}

void UInv_InventoryComponent::Client_PrintClearedScore_Implementation(int32 ClearedScore)
{
	const FString Message = FString::Printf(TEXT("Inventory cleared. Total score: %d"), ClearedScore);
	UKismetSystemLibrary::PrintString(this, Message, true, true, FLinearColor::Yellow, 4.0f);
	UE_LOG(LogInventory, Log, TEXT("%s"), *Message);
}

void UInv_InventoryComponent::RequestEquipItem(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip)
{
	Server_RequestEquipItem(ItemToEquip, ItemToUnequip);
}

void UInv_InventoryComponent::Server_RequestEquipItem_Implementation(UInv_InventoryItem* ItemToEquip, UInv_InventoryItem* ItemToUnequip)
{
	// Debug logging: owner, authority, caller context, and items involved
	AActor* OwnerActor = GetOwner();
	const bool bOwnerHasAuthority = OwnerActor ? OwnerActor->HasAuthority() : false;
	const ENetRole LocalRole = OwnerActor ? OwnerActor->GetLocalRole() : ROLE_None;
	const TCHAR* RoleName = LocalRole == ROLE_Authority ? TEXT("Authority") :
						   (LocalRole == ROLE_AutonomousProxy ? TEXT("AutonomousProxy") :
						   (LocalRole == ROLE_SimulatedProxy ? TEXT("SimulatedProxy") : TEXT("None")));
	UE_LOG(LogInventory, Warning, TEXT("Server_RequestEquipItem_Implementation called on Owner=%s, HasAuthority=%d, LocalRole=%s, ItemToEquip=%p (%s), ItemToEquipType=%s, ItemToUnequip=%p (%s)"),
		OwnerActor ? *OwnerActor->GetName() : TEXT("null"),
		bOwnerHasAuthority,
		RoleName,
		ItemToEquip,
		(ItemToEquip && IsValid(ItemToEquip)) ? *ItemToEquip->GetName() : TEXT("null"),
		(ItemToEquip && IsValid(ItemToEquip)) ? *ItemToEquip->GetItemManifest().GetItemType().ToString() : TEXT("None"),
		ItemToUnequip,
		(ItemToUnequip && IsValid(ItemToUnequip)) ? *ItemToUnequip->GetName() : TEXT("null")
	);
	
	// Equipment Component will listen to these delegates
	OnItemEquipped.Broadcast(ItemToEquip);
	OnItemUnequipped.Broadcast(ItemToUnequip);
	
	// Multicast_RequestEquipItem(ItemToEquip, ItemToUnequip);

	// if (!IsValid(Item)) return;
	//
	// // Validate: does item even have equipment fragment?
	// FInv_EquipmentFragment* EquipFrag =
	// 	Item->GetItemManifestMutable().GetFragmentOfTypeMutable<FInv_EquipmentFragment>();
	//
	// if (!EquipFrag) return;
}

void UInv_InventoryComponent::Multicast_RequestEquipItem_Implementation(UInv_InventoryItem* ItemToEquip,
	UInv_InventoryItem* ItemToUnequip)
{
	// Debug logging: show where this multicast is executing (server/client role and owner)
	AActor* OwnerActor = GetOwner();
	const ENetRole LocalRole = OwnerActor ? OwnerActor->GetLocalRole() : ROLE_None;
	const TCHAR* RoleName = LocalRole == ROLE_Authority ? TEXT("Authority") :
						   (LocalRole == ROLE_AutonomousProxy ? TEXT("AutonomousProxy") :
						   (LocalRole == ROLE_SimulatedProxy ? TEXT("SimulatedProxy") : TEXT("None")));
	UE_LOG(LogInventory, Warning, TEXT("Multicast_RequestEquipItem_Implementation executing on Owner=%s, LocalRole=%s, ItemToEquip=%p (%s)"),
		OwnerActor ? *OwnerActor->GetName() : TEXT("null"), RoleName,
		ItemToEquip, (ItemToEquip && IsValid(ItemToEquip)) ? *ItemToEquip->GetName() : TEXT("null")
	);

	// Equipment Component will listen to these delegates
	OnItemEquipped.Broadcast(ItemToEquip);
	OnItemUnequipped.Broadcast(ItemToUnequip);
}

void UInv_InventoryComponent::SetHotbarSlot(UInv_InventoryItem* Item, int32 SlotIndex)
{
	Server_SetHotbarSlot(Item, SlotIndex);
}

void UInv_InventoryComponent::Server_SetHotbarSlot_Implementation(UInv_InventoryItem* Item, int32 SlotIndex)
{
	OnHotbarSlotChanged.Broadcast(Item, SlotIndex);
}

TArray<UInv_InventoryItem*> UInv_InventoryComponent::GetAllInventoryItems() const
{
	return InventoryList.GetAllItems();
}

void UInv_InventoryComponent::ToggleInventoryMenu()
{
	if (bInventoryMenuOpen)
	{
		CloseInventoryMenu();
	}
	else
	{
		OpenInventoryMenu();
	}
	OnInventoryMenuToggled.Broadcast(bInventoryMenuOpen);
}

void UInv_InventoryComponent::AddRepSubObj(UObject* SubObj)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObj))
	{
		AddReplicatedSubObject(SubObj);
	}
}

void UInv_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	ConstructInventory();
}

void UInv_InventoryComponent::ConstructInventory()
{
	OwningController = Cast<APlayerController>(GetOwner());
	checkf(OwningController.IsValid(), TEXT("Inventory Component should have a Player Controller as Owner."))
	if (!OwningController->IsLocalController()) return;

	InventoryMenu = CreateWidget<UInv_InventoryBase>(OwningController.Get(), InventoryMenuClass);
	InventoryMenu->AddToViewport(2);
	CloseInventoryMenu();
}

void UInv_InventoryComponent::OpenInventoryMenu()
{
	if (!IsValid(InventoryMenu)) return;

	InventoryMenu->SetVisibility(ESlateVisibility::Visible);
	bInventoryMenuOpen = true;

	if (!OwningController.IsValid()) return;

	FInputModeGameAndUI InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(true);
}

void UInv_InventoryComponent::CloseInventoryMenu()
{
	if (!IsValid(InventoryMenu)) return;

	InventoryMenu->SetVisibility(ESlateVisibility::Collapsed);
	bInventoryMenuOpen = false;

	if (!OwningController.IsValid()) return;

	FInputModeGameOnly InputMode;
	OwningController->SetInputMode(InputMode);
	OwningController->SetShowMouseCursor(false);
}
