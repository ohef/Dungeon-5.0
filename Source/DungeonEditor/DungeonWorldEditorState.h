// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Logic/DungeonGameState.h"
#include "UObject/Object.h"
#include "UnitSchema.h"

#include "DungeonWorldEditorState.generated.h"

/**
 * 
 */
USTRUCT()
struct DUNGEONEDITOR_API FDungeonWorldEditorState 
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FDungeonWorldState WorldState;

	UPROPERTY(EditAnywhere)
	FDungeonLogicUnitRow CurrentUnit;
	
	UPROPERTY(EditAnywhere)
	UUnitSchema* Table;
};
