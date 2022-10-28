// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonMainWidget.h"

#include "Dungeon/DungeonGameModeBase.h"
#include "Dungeon/Lenses/model.hpp"

UDungeonMainWidget::UDungeonMainWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

bool UDungeonMainWidget::Initialize()
{
  Super::Initialize();

  interactionCursor = lager::view(worldStoreLens, GetWorld())->zoom(interactionStateLens).make();
  interactionCursor.bind([this](auto x) { return this->React(x); });
  
  return true;
}

void UDungeonMainWidget::React(TOptional<FMainMenu> visible)
{
  if (visible.IsSet())
  {
    MainMapMenu->SetVisibility(ESlateVisibility::Visible);
  }
  else
  {
    MainMapMenu->SetVisibility(ESlateVisibility::Collapsed);
  }
}
