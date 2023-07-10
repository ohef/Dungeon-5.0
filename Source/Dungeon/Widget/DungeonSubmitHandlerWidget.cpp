// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonSubmitHandlerWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "UMG/Public/Animation/WidgetAnimation.h"

UDungeonSubmitHandlerWidget::UDungeonSubmitHandlerWidget(const FObjectInitializer& Initializer) : Super(Initializer)
{
  static ConstructorHelpers::FObjectFinder<UMaterialInterface> TimingCircleMaterial(
    TEXT("Material'/Game/Blueprints/TimingCircle.TimingCircle'"));

  CircleMaterial = TimingCircleMaterial.Object;
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
    double innerRadius = 1.0 - (interval.interval.Min / TimelineLength);
    dynamicMaterial->SetScalarParameterValue(FName(TEXT("InnerRadius")), innerRadius * .5);
    double outerRadius = (1.0 - interval.interval.Max / TimelineLength);
    dynamicMaterial->SetScalarParameterValue(FName(TEXT("OuterRadius")), outerRadius * .5);
    dynamicMaterial->SetScalarParameterValue(FName(TEXT("Opacity")), 1.f);
    // dynamicMaterial->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor::Black );
    circleWidget->SetBrushFromMaterial(dynamicMaterial);
  }

  InitialOuterCircleSize = circleSize;
  OuterCircle->SetRenderOpacity(1.0);
  SetVisibility(ESlateVisibility::Visible);
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

void UDungeonSubmitHandlerWidget::NativeOnInitialized()
{
  Super::NativeOnInitialized();
  
  FWidgetAnimationDynamicEvent delegate = FWidgetAnimationDynamicEvent();
  delegate.BindDynamic(this, &UDungeonSubmitHandlerWidget::Stop);
  this->BindToAnimationFinished(OuterDissappear, delegate);
  
  Cast<UCanvasPanelSlot>(OuterCircle->Slot)->SetZOrder(2);
}