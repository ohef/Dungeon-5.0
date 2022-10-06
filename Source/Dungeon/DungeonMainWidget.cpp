// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonMainWidget.h"

UDungeonMainWidget::UDungeonMainWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

bool UDungeonMainWidget::Initialize()
{
  Super::Initialize();
  MainMapMenu->VisibilityDelegate.BindDynamic(this, &UDungeonMainWidget::GetMainVis);
  return true;
}


ESlateVisibility UDungeonMainWidget::GetMainVis()
{
  return MapMenuVisibility;
}