// Fill out your copyright notice in the Description page of Project Settings.

#include "UnitSchemaFactory.h"

#include "UnitSchema.h"

UUnitSchemaFactory::UUnitSchemaFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = true;
	SupportedClass = UUnitSchema::StaticClass();
}

UObject* UUnitSchemaFactory::FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags,
                                              UObject* Context, FFeedbackContext* Warn)
{
	check(Class && Class->IsChildOf(UUnitSchema::StaticClass()));
	return NewObject<UUnitSchema>(Parent, Class, Name, Flags);
}

bool UUnitSchemaFactory::DoesSupportClass(UClass* Class)
{
	return Super::DoesSupportClass(Class);
}
