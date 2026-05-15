// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HealthSystemInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UHealthSystemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * From AI, some unreal BP magic:
 * 1. The Mapping Rules
 *When you mark a function with UFUNCTION(BlueprintCallable) or BlueprintNativeEvent, Unreal interprets the parameters as follows:
 *
 * C++ Syntax						Blueprint Pin Location									Meaning
 * float Var							Left (Input)						Pass by Value. A copy is made.
 * const float& Var						Left (Input)						Pass by Const Reference. Efficient (no copy), but read-only.
 * float& Var							Right (Output)						Pass by Reference. Unreal assumes you want to return a value here.
 * return float							Right (Output)						The standard return value (usually named "ReturnValue").
 * 
 */
class UST_FYP_API IHealthSystemInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// "const = 0" makes it a pure C++ virtual function, but for BP implementation we use BlueprintNativeEvent.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Health|Getter")
	float IGetCurrentHP() const;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Health|Getter")
	float IGetMaxHP() const;
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Health|Getter")
	bool IGetIsHealthDepleted() const;

	// Some funny Unreal wizardry: Reference input in Interface makes them output pins (Return values).
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Health|ActionRequest")
	void IApplyDirectDmg(float Damage, TSubclassOf<UDamageType> DamageType, AController* InstigatingController, AActor* InstigatingActor, bool& DmgSuccess, float& FinalHealth, bool& IsHealthDepleted);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Health|ActionRequest")
	void IApplyDirectHeal(float Heal, TSubclassOf<UDamageType> HealType, AController* InstigatingController, AActor* InstigatingActor, bool& HealSuccess, float& FinalHealth, bool& IsHealthDepleted);
};
