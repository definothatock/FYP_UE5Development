// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MP_SyncTestingAattachment.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class UST_FYP_API AMP_SyncTestingAattachment : public AActor
{
	GENERATED_BODY()

public:
	AMP_SyncTestingAattachment();
	virtual void Tick(float DeltaTime) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere) // let other dev to see this in editor
	TObjectPtr<UStaticMeshComponent> SphereMesh;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> SphereCollision;
};
