// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "DungeonMainWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class DUNGEON_API UDungeonMainWidget : public UUserWidget
{
  GENERATED_BODY()

  DECLARE_DELEGATE_OneParam(FOnFocusLost, const FFocusEvent&)

public:
  FOnFocusLost FocusLost;

  UPROPERTY(meta=(BindWidgetAnim), Transient, EditAnywhere, BlueprintReadWrite)
  UWidgetAnimation* OnStart;
  
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UVerticalBox> MainMenu;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Attack;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Wait;
  UPROPERTY(meta=(BindWidget), EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UButton> Move;
};
