// Fill out your copyright notice in the Description page of Project Settings.


#include "UDungeonBlueprintLibrary.h"
#include "DungeonGameModeBase.h"

ADungeonGameModeBase* UUDungeonBlueprintLibrary::GetDungeonGamemode()
{
	return GEngine->GetWorld()->GetAuthGameMode<ADungeonGameModeBase>();
}
