// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonSubmitHandlerWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Dungeon/SingleSubmitHandler.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetMaterialLibrary.h"

UDungeonSubmitHandlerWidget::UDungeonSubmitHandlerWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
  static ConstructorHelpers::FObjectFinder<UMaterialInterface> NewMaterial6(
    TEXT("Material'/Game/Blueprints/NewMaterial6.NewMaterial6'"));

  CircleMaterial = NewMaterial6.Object;
}

void UDungeonSubmitHandlerWidget::RenderProperties(TArray<FIntervalPriority> IntervalPrioritiess, float TimelineLengthh,
  float PlaybackPositionn)
{
  this->IntervalPriorities = IntervalPrioritiess;
  this->TimelineLength = TimelineLengthh;
  this->PlaybackPosition = PlaybackPositionn;

  FVector2D circleSize = FVector2D{500, 500};

  for (auto AddedCircle : addedCircles)
  {
    AddedCircle->RemoveFromParent();
  }
  addedCircles.Empty();
  
  for (auto interval : IntervalPriorities)
  {
    UBorder* circleWidget = NewObject<UBorder>( this, UBorder::StaticClass());
    UPanelSlot* Src = Container->AddChild(circleWidget);
    addedCircles.Add(Src->Content);
    UCanvasPanelSlot* PanelSlot = CastChecked<UCanvasPanelSlot>(Src);
    PanelSlot->SetSize(circleSize);
    PanelSlot->SetPosition({0, 0});
    PanelSlot->SetAlignment({0.5, 0.5});
    auto dynamicMaterial = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, CircleMaterial);
    double minR = 1.0 - (interval.Min / TimelineLength);
    dynamicMaterial->SetScalarParameterValue(FName(TEXT("InnerRadius")), minR * .5);
    double maxR = (1.0 - interval.Max / TimelineLength);
    dynamicMaterial->SetScalarParameterValue(FName(TEXT("OuterRadius")), maxR * .5);
    circleWidget->SetBrushFromMaterial(dynamicMaterial);
    circleWidget->SetBrushColor(FLinearColor::White.CopyWithNewOpacity(.25));
  }

  InitialOuterCircleSize = circleSize;
}

void UDungeonSubmitHandlerWidget::Stop()
{
  for (auto AddedCircle : addedCircles)
  {
    AddedCircle->RemoveFromParent();
  }
  addedCircles.Empty();
  
  OuterCircle->SetRenderOpacity(1.0);
  this->SetVisibility(ESlateVisibility::Collapsed);
}

void UDungeonSubmitHandlerWidget::HandleHit()
{
  PlayAnimation(OuterDissappear, 0, 1, EUMGSequencePlayMode::Forward, 1,true);
}

void UDungeonSubmitHandlerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
  Super::NativeTick(MyGeometry, InDeltaTime);
}
