// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBox.h"
#include "DungeonUserWidget.generated.h"

struct State
{
  bool MenuVisibilty;
};

/**
 * 
 */
UCLASS(Abstract)
class DUNGEON_API UDungeonUserWidget : public UUserWidget
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
  TWeakObjectPtr<UWidget> Move;

  void Update(State state);
};
