// Fill out your copyright notice in the Description page of Project Settings.


#include "MP/MP_SyncTestingCube.h"


// Sets default values
AMP_SyncTestingCube::AMP_SyncTestingCube()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	/*
	 *The below options are selectable in editor, just that doing it here will set default
	 */
	
	// NetLoadOnClient means the actor will be loaded on clients when the level is loaded
	bNetLoadOnClient = true;
	// Replicates means the actor will be replicated to clients
	bReplicates = true; // bReplicate() called on non-init (which is here) actors will do nothing, it is for runtime
	// ReplicatingMovement means the actor's movement will be replicated to clients
	SetReplicatingMovement(true); // NOT using SetReplicateMovement() here, is vir func, calling here is not going to use the override
}

// Called when the game starts or when spawned
void AMP_SyncTestingCube::BeginPlay()
{
	Super::BeginPlay();

	const bool bAuth = HasAuthority();
	const ENetRole LocalRole = GetLocalRole();
	
}

// Called every frame
void AMP_SyncTestingCube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

