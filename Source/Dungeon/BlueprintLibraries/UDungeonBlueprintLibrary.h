// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UDungeonBlueprintLibrary.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEON_API UUDungeonBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    /** Starts an analytics session without any custom attributes specified */
    UFUNCTION(BlueprintPure, Category="Dungeon")
    static ADungeonGameModeBase* GetDungeonGamemode();
};
