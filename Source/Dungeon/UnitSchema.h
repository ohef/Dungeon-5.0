// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/Object.h"
#include "UnitSchema.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class DUNGEON_API UUnitSchema : public UDataTable
{
	GENERATED_BODY()
	
public:
	UUnitSchema();
	virtual ~UUnitSchema() = default;
};
