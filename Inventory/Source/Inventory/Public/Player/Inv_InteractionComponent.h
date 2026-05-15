#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Inv_InteractionComponent.generated.h"

class UInv_HotbarComponent;
class UInv_InventoryComponent;
class UInputMappingContext;
class UInputAction;
class UInv_HUDWidget;

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent), Blueprintable)
class INVENTORY_API UInv_InteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInv_InteractionComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventoryAndSaveScore();

protected:
	virtual void OnRegister() override;
	
	UFUNCTION()
	void OnPawnControllerChanged(
		APawn* Pawn,
		AController* OldController,
		AController* NewController
	);
	
private:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void PrimaryInteract();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleEquip(int32 Index);
	
	// void CreateHUDWidget();
	void TraceForItem();

	TWeakObjectPtr<UInv_InventoryComponent> InventoryComponent;
	TWeakObjectPtr<UInv_HotbarComponent> HotbarComponent;

	// UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	// TObjectPtr<UInputMappingContext> DefaultIMC;
	//
	// UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	// TObjectPtr<UInputAction> PrimaryInteractAction;
	//
	// UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	// TObjectPtr<UInputAction> ToggleInventoryAction;
	//
	// UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	// TSubclassOf<UInv_HUDWidget> HUDWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UInv_HUDWidget> HUDWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	double TraceLength = 500;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	double GrabRadius = 5;

	FCollisionShape Sphere;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TEnumAsByte<ECollisionChannel> ItemTraceChannel;

	TWeakObjectPtr<APlayerController> PC;
	TWeakObjectPtr<APawn> OwningPawn;
	TWeakObjectPtr<AActor> ThisActor;
	TWeakObjectPtr<AActor> LastActor;
};
