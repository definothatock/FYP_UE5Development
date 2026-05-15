// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPC_Base.generated.h"

//
// This Class is the BASE class of all main NPC.
// This thin Parent is meant to be the bridge to all NPC, to contain common tools / funcs that are used by all NPC.
// DO NOT MAKE INDIVIDUAL-SPECIFIC EDIT HERE !!!
//

UCLASS()
class UST_FYP_API ANPC_Base : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ANPC_Base();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
