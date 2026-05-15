// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/NPC/SharedData/DroneFlightMovement.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"


// Sets default values for this component's properties
UDroneFlightMovement::UDroneFlightMovement()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UDroneFlightMovement::BeginPlay()
{
	Super::BeginPlay();

	

	OwnerCharacter = Cast<ACharacter>(GetOwner());

	/* I dont really need this, just set it in editor.

	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);

		OwnerCharacter->GetCharacterMovement()->BrakingDecelerationFlying = 1000.f;
	}
	
	*/
	
}


// Called every frame
/*
 * Logic:
 * if received impact, apply force in the exact same direction as the impact, which is most likely going against the current direction.
 * if center receive impact, apply extra force that is not against the current direction, but to 'push' self along the wall.
 *
 * Limitation: Does not work for room traversing. That requires a dedicated pathfinding (octree). Or fake levitation with nevmesh.
*/
void UDroneFlightMovement::TickComponent(float DeltaTime, ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


    if (!IsFlightActive || !OwnerCharacter)
    	{ return; }

	/* ----- select Destination mode ----- */
	FVector TargetLocation;
	if (IsMoveToActor)
	{
		if (!CurrentTarget) // Target destroyed. 
		{
			StopFlight();
			OnMoveFinished.Broadcast(false);
			return;
		}
		TargetLocation = CurrentTarget->GetActorLocation();
	}
	else
	{
		TargetLocation = FixedTargetLocation;
	}
	
    FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	
	
    /* ----- Check reached ----- */
	float Distance = FVector::Dist(CurrentLocation, TargetLocation);
    if (Distance <= AcceptanceRadius)
    {
        StopFlight();
        OnMoveFinished.Broadcast(true);
        return;
    }
	

    /* ----- movement Steering (do not confuse with focus!) ----- */
    FVector DesiredDirection = (TargetLocation - CurrentLocation).GetSafeNormal();
	FVector RawAvoidanceVector = FVector::ZeroVector;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	
	
    /* ----- Forward Obstacle Avoidance (excluded Z axis) ----- */

/* OLD: Single forward trace, way too jittery
    FHitResult HitForward;
	
	// If self is moving fast enough, look where self is going. If stopped, look where self wants to go. (GetSafeNormal limitation)
	FVector TraceDirection = OwnerCharacter->GetVelocity().SizeSquared() > 10.f ? OwnerCharacter->GetVelocity().GetSafeNormal() : DesiredDirection;
    FVector ForwardEnd = CurrentLocation + (TraceDirection * CenterWhiskerLength);
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerCharacter);

	// sweep and check visible thing blocking the way. // might be stupid rn cuz its only checking one and only one thing.
    bool IsHitSomething = GetWorld()->SweepSingleByChannel(
        HitForward, CurrentLocation, ForwardEnd, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(ClearanceRadius), QueryParams
    );
	
#if UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
	if (IsDrawDebugLines)
	{
		DrawDebugSphere(this->GetWorld(), CurrentLocation, ClearanceRadius, 12, FColor::Red, false,-1, 0.0f, 1.0f);
		DrawDebugSphere(this->GetWorld(), ForwardEnd, ClearanceRadius, 12, FColor::Red, false,-1, 0.0f, 1.0f);
		DrawDebugLine(GetWorld(), CurrentLocation, ForwardEnd, FColor::Red, false, -1, 0.0f, 1.0f);
	}
#endif

    if (IsHitSomething)
    {
        // Push away from the object's normal
    	FVector AvoidanceForce = HitForward.ImpactNormal * AvoidanceForceMultiplier;
    	AvoidanceForce.Z = 0.0f; // make the Force 2D here, altitude tracker should handle Z.
        DesiredDirection += AvoidanceForce;
    	
#if UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
    	if (IsDrawDebugLines){DrawDebugLine(GetWorld(), HitForward.ImpactPoint, HitForward.ImpactPoint + AvoidanceForce.GetSafeNormal() * AvoidanceForceMultiplier, FColor::Red, false, 0.0f, 0.0f, 3.0f);}
#endif
    	}
*/
	
	// If self is moving fast enough, look where self is going. If stopped, look where self wants to go. (GetSafeNormal limitation)
	// FVector BaseForward = OwnerCharacter->GetVelocity().SizeSquared() > 10.f ? OwnerCharacter->GetVelocity().GetSafeNormal() : DesiredDirection;

	// --- Flatten Horizontal direction data ---
	
	FVector RawVelocity = OwnerCharacter->GetVelocity();
	FVector FlattenedVelocity = FVector(RawVelocity.X, RawVelocity.Y, 0.0f);

	// fat and 2D version of the commented one
	FVector BaseForward2D;
	if (FlattenedVelocity.SizeSquared() > 10.f)
	{
		BaseForward2D = FlattenedVelocity.GetSafeNormal();
	}
	else
	{
		FVector DesiredDirection2D = (TargetLocation - CurrentLocation);
		DesiredDirection2D.Z = 0.0f;
		BaseForward2D = DesiredDirection2D.GetSafeNormal();
        
		// GetSafeNormal again, just in case
		if (BaseForward2D.IsZero())
		{
			BaseForward2D = OwnerCharacter->GetActorForwardVector();
			BaseForward2D.Z = 0.0f;
			BaseForward2D.Normalize();
		}
	}

	// --- Center Probe ---
	
    FHitResult HitCenterResult;
    FVector CenterEnd = CurrentLocation + (BaseForward2D * CenterClearanceLength);
    bool IsHitCenter = GetWorld()->SweepSingleByChannel(
        HitCenterResult, CurrentLocation, CenterEnd, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(ClearanceRadius), QueryParams
    );

	FVector WallTangent =  FVector::ZeroVector;
    if (IsHitCenter)
    {
    	// Standard Repulsion
        FVector CenterForce = HitCenterResult.ImpactNormal;
        float DistanceFactor = 1.0f - FMath::Clamp(HitCenterResult.Distance / CenterClearanceLength, 0.0f, 1.0f);	// Scale force by how close
        RawAvoidanceVector += (CenterForce * CenterAvoidanceForceMultiplier * (1.0f + DistanceFactor));

    	// --- Fix for directly facing walls (Oscillation hell)
    	// apply PUSHING force parallel to the surface.
        
    	// Get the direction of the wall surface (Tangent, to left or to right)
    	WallTangent = FVector::CrossProduct(FVector::UpVector, HitCenterResult.ImpactNormal).GetSafeNormal();
        
    	// Check if this tangent points towards target or away from it
    	FVector ToTarget = (TargetLocation - CurrentLocation).GetSafeNormal();
    	float DotToTarget = FVector::DotProduct(WallTangent, ToTarget);

    	// pick the tangent direction that is the closest to goal (flip it)
    	if (DotToTarget < 0.0f)
    	{
    		WallTangent *= -1.0f;
    	}

    	// Apply the sliding force. 
    	// Multiply by DistanceFactor so it slides harder the closer it gets to the wall.
    	RawAvoidanceVector += WallTangent * CenterAvoidanceForceMultiplier * WallSlidingForceMultiplier * (1.0f + DistanceFactor);
    }

    // --- L&R Probe ---
    
    // Calculate Rotated Vectors
    FVector LeftDir = BaseForward2D.RotateAngleAxis(-WhiskerSpreadAngle, FVector::UpVector);
    FVector RightDir = BaseForward2D.RotateAngleAxis(WhiskerSpreadAngle, FVector::UpVector);

    FHitResult HitLeft, HitRight;
    FVector LeftEnd = CurrentLocation + (LeftDir * SideWhiskerLength);
    FVector RightEnd = CurrentLocation + (RightDir * SideWhiskerLength);

    bool bHitLeft = GetWorld()->LineTraceSingleByChannel(HitLeft, CurrentLocation, LeftEnd, ECC_Visibility, QueryParams);
    bool bHitRight = GetWorld()->LineTraceSingleByChannel(HitRight, CurrentLocation, RightEnd, ECC_Visibility, QueryParams);

    if (bHitLeft)
    {
        FVector LeftPush = HitLeft.ImpactNormal;
        float DistanceFactor = 1.0f - FMath::Clamp(HitLeft.Distance / SideWhiskerLength, 0.0f, 1.0f);
        RawAvoidanceVector += (LeftPush * SideAvoidanceForceMultiplier * (1.0f + DistanceFactor));
    }

    if (bHitRight)
    {
        FVector RightPush = HitRight.ImpactNormal;
        float DistanceFactor = 1.0f - FMath::Clamp(HitRight.Distance / SideWhiskerLength, 0.0f, 1.0f);
        RawAvoidanceVector += (RightPush * SideAvoidanceForceMultiplier * (1.0f + DistanceFactor));
    }

    // --- Accumulation ---
	PersistenceSmoothedAvoidance = FMath::VInterpTo(PersistenceSmoothedAvoidance, RawAvoidanceVector, DeltaTime, AvoidanceInterpSpeed);
    if (!RawAvoidanceVector.IsZero())
    {
        PersistenceSmoothedAvoidance.Z = 0.0f; // make the Force 2D here, altitude tracker should handle Z.
        
        DesiredDirection += PersistenceSmoothedAvoidance;
    }

#if UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
    if (IsDrawDebugLines)
    {
        DrawDebugLine(GetWorld(), CurrentLocation, CenterEnd, IsHitCenter ? FColor::Red : FColor::Green, false, -1, 0, 2.0f);
    	DrawDebugSphere(GetWorld(), CenterEnd, ClearanceRadius, 12, IsHitCenter ? FColor::Red : FColor::Green, false,-1, 0.0f, 1.0f);
    	
        DrawDebugLine(GetWorld(), CurrentLocation, LeftEnd, bHitLeft ? FColor::Red : FColor::Green, false, -1, 0, 1.0f);
        DrawDebugLine(GetWorld(), CurrentLocation, RightEnd, bHitRight ? FColor::Red : FColor::Green, false, -1, 0, 1.0f);
        
        if(!RawAvoidanceVector.IsZero())
        {
        	if (!PersistenceSmoothedAvoidance.IsZero() && HitCenterResult.ImpactPoint != FVector::ZeroVector)
        	{
        		DrawDebugSphere(GetWorld(), HitCenterResult.ImpactPoint, 10, 12, FColor::Orange, false, -1.0f, 0.0f);
        		DrawDebugLine(GetWorld(), HitCenterResult.ImpactPoint, CurrentLocation + (WallTangent.GetSafeNormal() * 200.f), FColor::Orange, false, -1, 0, 10.0f);
        	}
        	if (!RawAvoidanceVector.IsZero())
        	{ DrawDebugLine(GetWorld(), CurrentLocation, CurrentLocation + (RawAvoidanceVector.GetSafeNormal() * 200.f), FColor::Purple, false, -1, 0, 2.0f); }
        	if (!PersistenceSmoothedAvoidance.IsZero())
        	{ DrawDebugLine(GetWorld(), CurrentLocation, CurrentLocation + (PersistenceSmoothedAvoidance.GetSafeNormal() * 200.f), FColor::Yellow, false, -1, 0, 4.0f); }
        	}
    }
#endif

	
    /* ----- Ground Height Check ----- */
	
    FHitResult HitGround;
    FVector DownEnd = CurrentLocation - FVector(0, 0, GroundHeight_UpperThrustHold);

    bool bHitGround = GetWorld()->LineTraceSingleByChannel(
        HitGround, CurrentLocation, DownEnd, ECC_Visibility, QueryParams
    );
	
	float DistToGround = HitGround.Distance;
    if (bHitGround)
    {
        if (DistToGround < GroundHeight_LowerThrustHold)
        {
            /* altitude. proportional pull up. */
            float PushStrength = 1.0f - (DistToGround / GroundHeight_LowerThrustHold); 
            DesiredDirection.Z += (PushStrength * GroundHeight_CalibrationMultiplier);
        }
    }
	else // flew higher than Max Height
    { 
		if ( CurrentLocation.Z > (TargetLocation.Z + 300.0f) ) // much higher than the target
		{
			DesiredDirection.Z -= (1.0f * GroundHeight_CalibrationMultiplier);
		}

    }
		
#if UE_BUILD_DEVELOPMENT || UE_BUILD_DEBUG
	if (IsDrawDebugLines) {DrawDebugLine(GetWorld(), CurrentLocation, DownEnd, FColor::Red, false, 0.0f, 0.0f, 1.0f);}
#endif

    /* ----- Apply Final Input ----- */
    DesiredDirection.Normalize();
    OwnerCharacter->AddMovementInput(DesiredDirection, 1.0f);

	if (IsMoveToActor == false && IsInterpToDestination == false)
	{
		// look at where self is going
		FRotator NewRot = FMath::RInterpTo(OwnerCharacter->GetActorRotation(), DesiredDirection.Rotation(), DeltaTime, RotInterpSpeed);
		OwnerCharacter->SetActorRotation(NewRot);
	}
}


void UDroneFlightMovement::MoveToActor(AActor* TargetActor)
{
	if(!TargetActor)
	{
		OnMoveFinished.Broadcast(false);
		return;
	}
	
	CurrentTarget = TargetActor;
	IsMoveToActor = true;
	IsFlightActive = true;
}


void UDroneFlightMovement::MoveToLocation(FVector InLocation)
{
	FixedTargetLocation = InLocation;
	CurrentTarget = nullptr;
	IsMoveToActor = false;
	IsFlightActive = true;
}


void UDroneFlightMovement::StopFlight()
{
	IsFlightActive = false;
	CurrentTarget = nullptr;

	PersistenceSmoothedAvoidance = FVector::ZeroVector;
	
	if (OwnerCharacter)
	{
		OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
	}
}


