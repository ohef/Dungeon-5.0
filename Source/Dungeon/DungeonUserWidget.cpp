// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonUserWidget.h"

State lastState;

void UDungeonUserWidget::Update(State state)
{
  this->SetVisibility(state.MenuVisibilty ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
  PlayAnimation(TheAnimationWoo, 0, 1, EUMGSequencePlayMode::Forward, 14.0, false);
}

FReply UDungeonUserWidget::NativeOnFocusReceived(const FGeometry& MovieSceneBlends, const FFocusEvent& InFocusEvent)
{
  FReply SceneBlends = Super::NativeOnFocusReceived(MovieSceneBlends, InFocusEvent);
  return SceneBlends;
}
