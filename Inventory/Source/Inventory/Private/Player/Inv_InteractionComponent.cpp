

#include "Player/Inv_InteractionComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "HotbarManagement/Inv_HotbarComponent.h"
#include "Interaction/Inv_Highlightable.h"
#include "InventoryManagement/Components/Inv_InventoryComponent.h"
#include "Items/Components/Inv_ItemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/HUD/Inv_HUDWidget.h"

UInv_InteractionComponent::UInv_InteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	TraceLength = 500.0f;
	ItemTraceChannel = ECC_GameTraceChannel1;
}

void UInv_InteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TraceForItem();
}

void UInv_InteractionComponent::ToggleInventory()
{
	if (!InventoryComponent.IsValid()) return;
	InventoryComponent->ToggleInventoryMenu();

	if (!HUDWidget) return;
	
	if (InventoryComponent->IsMenuOpen())
	{
		HUDWidget->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		HUDWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UInv_InteractionComponent::ClearInventoryAndSaveScore()
{
	if (!InventoryComponent.IsValid()) return;
	InventoryComponent->Server_ClearInventoryAndSaveScore();
}

void UInv_InteractionComponent::OnRegister()
{
	Super::OnRegister();
	OwningPawn = Cast<APawn>(GetOwner());
    if (!OwningPawn.IsValid()) return;
	
	OwningPawn->ReceiveControllerChangedDelegate.AddDynamic(
		this,
		&UInv_InteractionComponent::OnPawnControllerChanged
	);
}

void UInv_InteractionComponent::OnPawnControllerChanged(APawn* Pawn, AController* OldController,
                                                        AController* NewController)
{
	if (!Pawn || !NewController) return;
	PC = Cast<APlayerController>(NewController);
	if (!PC.IsValid() || !PC->IsLocalController()) return;
	
	InventoryComponent = PC->FindComponentByClass<UInv_InventoryComponent>();
	HotbarComponent = PC->FindComponentByClass<UInv_HotbarComponent>();

	Sphere = FCollisionShape::MakeSphere(GrabRadius);
}

void UInv_InteractionComponent::PrimaryInteract()
{
	UE_LOG(LogTemp, Log, TEXT("PrimaryInteract"));
	if (!ThisActor.IsValid()) return;
	UInv_ItemComponent* ItemComp = ThisActor->FindComponentByClass<UInv_ItemComponent>();
	if (!IsValid(ItemComp) || !InventoryComponent.IsValid()) return;

	InventoryComponent->TryAddItem(ItemComp);
}

void UInv_InteractionComponent::ToggleEquip(int32 Index)
{
	if (!InventoryComponent.IsValid()) return;
	
	HotbarComponent->ToggleEquipItem(Index);
}

// void UInv_InteractionComponent::CreateHUDWidget()
// {
// 	if (!PC->IsLocalController()) return;
// 	HUDWidget = 
// 	CreateWidget<UInv_HUDWidget>(PC.Get(), HUDWidgetClass);
// 	if (IsValid(HUDWidget))
// 	{
// 		HUDWidget->AddToViewport();
// 	}
// }

void UInv_InteractionComponent::TraceForItem()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize / 2.f;
	FVector TraceStart;
	FVector Forward;
	if (!PC.IsValid()) return;
	if (!UGameplayStatics::DeprojectScreenToWorld(PC.Get(), ViewportCenter, TraceStart, Forward))
	{
		return;
	}
	const FVector TraceEnd = TraceStart + Forward * TraceLength;
	FHitResult HitResult;
	if (!Sphere.IsSphere()) return;
	// GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemTraceChannel);
	GetWorld()->SweepSingleByChannel(HitResult, TraceStart, TraceEnd, FQuat::Identity, ItemTraceChannel, Sphere);

	LastActor = ThisActor;
	ThisActor = HitResult.GetActor();

	if (!ThisActor.IsValid())
	{
		if (IsValid(HUDWidget)) HUDWidget->HidePickupMessage();
	}

	if (ThisActor == LastActor) return;

	if (ThisActor.IsValid())
	{
		// UE_LOG(LogTemp, Warning, TEXT("start tracing a new actor"));
		
		if (UActorComponent* Highlightable = ThisActor->FindComponentByInterface(UInv_Highlightable::StaticClass());
			IsValid(Highlightable))
		{
			IInv_Highlightable::Execute_Highlight(Highlightable);
		}
		
		UInv_ItemComponent* ItemComponent = ThisActor->FindComponentByClass<UInv_ItemComponent>();
		if (!IsValid(ItemComponent)) return;

		if (IsValid(HUDWidget)) HUDWidget->ShowPickupMessage(ItemComponent->GetPickupMessage());
	}

	if (LastActor.IsValid())
	{
		// UE_LOG(LogTemp, Warning, TEXT("stop tracing a new actor"));
		
		if (UActorComponent* Highlightable = LastActor->FindComponentByInterface(UInv_Highlightable::StaticClass());
			IsValid(Highlightable))
		{
			IInv_Highlightable::Execute_UnHighlight(Highlightable);
		}
	}
}
