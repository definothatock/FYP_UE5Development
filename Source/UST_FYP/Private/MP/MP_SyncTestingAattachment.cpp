// Fill out your copyright notice in the Description page of Project Settings.


#include "MP/MP_SyncTestingAattachment.h"

#include "Components/SphereComponent.h"


// Sets default values
AMP_SyncTestingAattachment::AMP_SyncTestingAattachment()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));
	
	SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>("SphereMesh");
	SphereMesh->SetupAttachment(RootComponent);

	SphereCollision = CreateDefaultSubobject<USphereComponent>("SphereCollision");
	SphereCollision->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AMP_SyncTestingAattachment::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMP_SyncTestingAattachment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMP_SyncTestingAattachment::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	// Only the server should handle the attachment logic
	if (HasAuthority())
	{
		AttachToActor(OtherActor, FAttachmentTransformRules::SnapToTargetIncludingScale);
	}
}

