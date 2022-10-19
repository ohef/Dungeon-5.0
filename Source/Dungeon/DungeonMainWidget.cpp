// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonMainWidget.h"

#include "DungeonGameModeBase.h"
#include "Utility/LagerIntegration.hpp"

const auto interactionStateLens = attr(&FDungeonWorldState::InteractionContext) | unreal_alternative<FMainMenu>;

UDungeonMainWidget::UDungeonMainWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

bool UDungeonMainWidget::Initialize()
{
  Super::Initialize();
  // MainMapMenu->VisibilityDelegate.BindDynamic(this, &UDungeonMainWidget::GetMainVis);
  
  interactionCursor = GetWorld()->GetAuthGameMode<ADungeonGameModeBase>()->store->zoom(interactionStateLens).make();
  this->React(interactionCursor.get());
  interactionCursor.watch([&](TOptional<FMainMenu> visible)
  {
    this->React(visible);
  });
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

ESlateVisibility UDungeonMainWidget::GetMainVis()
{
  return MapMenuVisibility;
}
