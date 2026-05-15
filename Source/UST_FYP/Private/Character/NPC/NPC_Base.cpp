// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/NPC/NPC_Base.h"

//
// This Class is the BASE class of all main NPC.
// This thin Parent is meant to be the bridge to all NPC, to contain common tools / funcs that are used by all NPC.
// DO NOT MAKE INDIVIDUAL-SPECIFIC EDIT HERE !!!
//

// Sets default values
ANPC_Base::ANPC_Base()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANPC_Base::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANPC_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANPC_Base::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

