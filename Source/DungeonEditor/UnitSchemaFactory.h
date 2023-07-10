// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "UObject/Object.h"
#include "UnitSchemaFactory.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class DUNGEONEDITOR_API UUnitSchemaFactory : public UFactory
{
	GENERATED_BODY()

public:
	UUnitSchemaFactory(const FObjectInitializer& ObjectInitializer);
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context,
	                          FFeedbackContext* Warn) override;

	virtual bool DoesSupportClass(UClass* Class) override;
};
