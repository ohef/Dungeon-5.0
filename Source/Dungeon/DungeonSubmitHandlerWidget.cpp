// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonSubmitHandlerWidget.h"

#include "SingleSubmitHandler.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TimelineComponent.h"
#include "Engine/Canvas.h"

void UDungeonSubmitHandlerWidget::HandleHit()
{
  PlayAnimation(OuterDissappear, 0, 1, EUMGSequencePlayMode::Forward, 4);
}

void UDungeonSubmitHandlerWidget::NativeOnInitialized()
{
  Super::NativeOnInitialized();
  InitialOuterCircleSize = CastChecked<UCanvasPanelSlot>(OuterCircle->Slot)->GetSize();

  UCanvasPanelSlot* CanvasPanelSlot = CastChecked<UCanvasPanelSlot>(InnerCircle->Slot);
  CanvasPanelSlot->SetSize(
    CanvasPanelSlot->GetSize() * (1.0 - singleSubmitHandler->pivot / singleSubmitHandler->totalLength));
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
