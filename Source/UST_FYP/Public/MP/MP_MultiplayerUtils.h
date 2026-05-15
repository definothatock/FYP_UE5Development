// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MP_MultiplayerUtils.generated.h"

/**
 * 
 */
UCLASS()
class UST_FYP_API UMP_MultiplayerUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	// this create a untility function that can be called from blueprints
	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utilities")
	static void PrintLocalNetRole(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer Utilities")
	static void PrintRemoteNetRole(AActor* Actor);
};
