﻿// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonSubmitHandlerWidget.h"

#include "SingleSubmitHandler.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TimelineComponent.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetMaterialLibrary.h"

UDungeonSubmitHandlerWidget::UDungeonSubmitHandlerWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
  static ConstructorHelpers::FObjectFinder<UMaterialInterface> NewMaterial6(
    TEXT("Material'/Game/Blueprints/NewMaterial6.NewMaterial6'"));

  CircleMaterial = NewMaterial6.Object;
}

void UDungeonSubmitHandlerWidget::HandleHit()
{
  PlayAnimation(OuterDissappear, 0, 1, EUMGSequencePlayMode::Forward, 1);
}

void UDungeonSubmitHandlerWidget::NativeOnInitialized()
{
  Super::NativeOnInitialized();
  FVector2D circleSize = FVector2D{500, 500};

  for (auto interval : singleSubmitHandler->handlers)
  {
    auto circleWidget = NewObject<UBorder>(
      this, UBorder::StaticClass());
    auto PanelSlot = CastChecked<UCanvasPanelSlot>(Container->AddChild(circleWidget));
    PanelSlot->SetSize(circleSize);
    PanelSlot->SetPosition({0, 0});
    PanelSlot->SetAlignment({0.5, 0.5});
    auto dynamicMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, CircleMaterial);
    float TimelineLength = singleSubmitHandler->totalLength;
    double minR = 1.0 - ( interval.Min / TimelineLength );
    dynamicMaterial->SetScalarParameterValue(FName(TEXT("InnerRadius")),minR * .5);
    double maxR = ( 1.0 - interval.Max / TimelineLength );
    dynamicMaterial->SetScalarParameterValue(FName(TEXT("OuterRadius")), maxR * .5);
    circleWidget->SetBrushFromMaterial(dynamicMaterial);
    circleWidget->SetBrushColor(FLinearColor::White.CopyWithNewOpacity(.25));
  }

  InitialOuterCircleSize = circleSize;
}

void UDungeonSubmitHandlerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  Super::NativeTick(MyGeometry, InDeltaTime);

  UCanvasPanelSlot* CanvasPanelSlot = CastChecked<UCanvasPanelSlot>(OuterCircle->Slot);
  if (this->singleSubmitHandler.IsValid())
  {
    FTimeline Timeline = this->singleSubmitHandler->timeline;
    CanvasPanelSlot->SetSize(
      InitialOuterCircleSize * (1.0 - (Timeline.GetPlaybackPosition() / Timeline.GetTimelineLength())));
  }
}
