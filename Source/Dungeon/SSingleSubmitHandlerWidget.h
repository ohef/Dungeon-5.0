// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SingleSubmitHandler.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class DUNGEON_API SSingleSubmitHandlerWidget : public SCompoundWidget
{
public:
  SLATE_BEGIN_ARGS(SSingleSubmitHandlerWidget)
    {
    }

    SLATE_ARGUMENT(USingleSubmitHandler*, SingleSubmitHandler)
    SLATE_ATTRIBUTE(const FSlateBrush*, CircleBrush)
  SLATE_END_ARGS()

  FVector2D viewportPosition  = FVector2D{0, 0};

  FMargin getMargin(FMargin initSize)
  {
    FTimeline Timeline = this->singleSubmitHandler->timeline;
    FMargin Margin = initSize * (1.0 - (Timeline.GetPlaybackPosition() / Timeline.GetTimelineLength()));
    Margin.Left = viewportPosition.X;
    Margin.Top = viewportPosition.Y;
    return Margin;
  };

  TWeakObjectPtr<USingleSubmitHandler> singleSubmitHandler;
  
  void Construct(const FArguments& InArgs);
};
