// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "UnitActionsMenu.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class DUNGEON_API UUnitActionsMenu : public UUserWidget
{
  GENERATED_BODY()
public:
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Attack;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Wait;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Move;
};
