// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
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

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  UWidgetAnimation* TheAnimationWoo;
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UVerticalBox> Parent;
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TWeakObjectPtr<UWidget> Child;

  void Update(State state);
protected:
  virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
};
