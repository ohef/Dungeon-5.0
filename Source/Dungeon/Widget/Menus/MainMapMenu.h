// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "MainMapMenu.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class DUNGEON_API UMainMapMenu: public UUserWidget
{
  GENERATED_BODY()

  DECLARE_DELEGATE_OneParam(FOnFocusLost, const FFocusEvent&)

public:
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> EndTurn;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Units;
};
