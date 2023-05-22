// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "lager/future.hpp"
#include "UObject/Object.h"
#include "PromiseFulfiller.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEON_API UPromiseFulfiller : public UObject
{
	GENERATED_BODY()
public:
	lager::promise promiseToFulfill = lager::promise::invalid().first;

	UFUNCTION()
	void HandleOnInterpToStop(const FHitResult& x, float y)
	{
		promiseToFulfill();
	};
	
	UFUNCTION()
	void HandleOnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
	{
		promiseToFulfill();
	};
};


