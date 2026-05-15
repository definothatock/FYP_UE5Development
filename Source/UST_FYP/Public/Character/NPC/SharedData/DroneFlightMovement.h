// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DroneFlightMovement.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFlightMoveFinished, bool, bSuccess);

/*
 * Roomba.
 * Since unreal has no actual flying/levitating movements (AIMoveTo), this is a custom one, making use of proximity avoidance.
 * Only works when the environment is logically infinite and open. will not work if there exists an encapsulated volume.
 * 
 * NoteToSelf:
 * - My inheritance is from ACharacter. So this will not work for simply Pawn.
 *	 Also, This will work (fight) with the UCharacterMovement to some extent. Remember to Tweak the settings in UCharacterMovement!
 *	 (especially change the movement type!)
 * - Also The drawing are very expensive! remember to 
 *				
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UST_FYP_API UDroneFlightMovement : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UDroneFlightMovement();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
						   FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;



	/* ----- Internal State ----- */
private:
	bool IsFlightActive = false;
    
	UPROPERTY()
	AActor* CurrentTarget = nullptr;

	UPROPERTY()
	bool IsMoveToActor = false;
	
	UPROPERTY()
	FVector FixedTargetLocation; 

	UPROPERTY()
	ACharacter* OwnerCharacter;

	UPROPERTY()
	FVector PersistenceSmoothedAvoidance;


	/* ----- Settings ----- */
protected:
	// Should be larger than MinGroundHeight, pythagoras still haunting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Settings")
	float AcceptanceRadius = 600.0f;

	// This is not an absolute boundary! it simply means that the system will attempt to recalibrate with proportional magnitude!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Settings")
	float GroundHeight_LowerThrustHold = 500.0f;
	// This is not an absolute boundary! it simply means that the system will attempt to recalibrate with proportional magnitude!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight Settings")
	float GroundHeight_UpperThrustHold = 900.0f;
	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float GroundHeight_CalibrationMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float CenterClearanceLength = 400.0f;
	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float ClearanceRadius = 25.0f;
	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float SideWhiskerLength = 350.0f;
	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float WhiskerSpreadAngle = 35.0f;


	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float CenterAvoidanceForceMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float WallSlidingForceMultiplier = 3.0f;
	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float SideAvoidanceForceMultiplier = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	bool IsInterpToDestination = true;
	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float AvoidanceInterpSpeed = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	float RotInterpSpeed = 2.0f;
	
	UPROPERTY(EditAnywhere, Category = "Flight Settings")
	bool IsDrawDebugLines = false;

	
	/* ----- Movements ----- */
public:
	// Simple avoidance levitation movement 
	UFUNCTION(BlueprintCallable, Category = "Drone Flight")
	void MoveToActor(AActor* TargetActor);

	// Simple avoidance levitation movement 
	UFUNCTION(BlueprintCallable, Category = "Drone Flight")
	void MoveToLocation(FVector InLocation);

	// literally.
	UFUNCTION(BlueprintCallable, Category = "Drone Flight")
	void StopFlight();

	// to inform finished (mostly for STT)
	UPROPERTY(BlueprintAssignable, Category = "Drone Flight")
	FOnFlightMoveFinished OnMoveFinished;

	// just in case, OnMoveFinished should be good enough
	UFUNCTION(BlueprintPure, Category = "Drone Flight")
	bool IsFlying() const { return IsFlightActive; }
};
