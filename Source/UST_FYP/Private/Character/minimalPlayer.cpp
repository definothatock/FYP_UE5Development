// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/minimalPlayer.h"

//Ad's includes
#include "kismet/GameplayStatics.h" // for UGameplayStatics::OpenLevel

// Sets default values
AminimalPlayer::AminimalPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AminimalPlayer::BeginPlay()
{
	Super::BeginPlay();

	const bool bAuth = HasAuthority();
	const ENetRole LocalRole = GetLocalRole();
	
}

// Called every frame
void AminimalPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AminimalPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

//
//Ad's functions
//


// Jump the server to the Lobby level
void AminimalPlayer::OpenLobby()
{
	//UWorld* World = GetWorld();
	//if (World) // add the "?listen" tag to make it a listen server
	//{
	//	World->ServerTravel("/Game/World/TestMap/ad_multiTestinglobby?listen");
	//}

	return;
}

// Jump the server to
void AminimalPlayer::CallOpenLevel(const FString& Adress)
{
	/*
	this: the World context object, in this case, the player character itself
	*Adress: FString is not the same with FName, but using *Adress will return a C style String, and that can be implicitly converted to FName
	*/
	/*UGameplayStatics::OpenLevel(this, *Adress);*/
	return;
}

// Jump the client to
void AminimalPlayer::CallClientTravel(const FString& Address)
{
	//APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	//if (PlayerController)
	//{
	//	PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
	//}

	return;
}
