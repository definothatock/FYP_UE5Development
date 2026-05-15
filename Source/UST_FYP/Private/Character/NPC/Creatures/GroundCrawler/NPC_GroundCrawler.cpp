// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/NPC/Creatures/GroundCrawler/NPC_GroundCrawler.h"


// Sets default values
ANPC_GroundCrawler::ANPC_GroundCrawler()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ANPC_GroundCrawler::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANPC_GroundCrawler::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ANPC_GroundCrawler::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

