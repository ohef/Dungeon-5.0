#include "SSingleSubmitHandlerWidget.h"

#include "SlateOptMacros.h"
#include "PropertyEditor/Private/SSingleProperty.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Layout/SConstraintCanvas.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSingleSubmitHandlerWidget::Construct(const FArguments& InArgs)
{
  singleSubmitHandler = MakeWeakObjectPtr(InArgs._SingleSubmitHandler);

  ChildSlot[
    SNew(SConstraintCanvas)
    + SConstraintCanvas::Slot()
      .Offset(MakeAttributeRaw(this, &SSingleSubmitHandlerWidget::getMargin, FMargin(0.0, 0.0, 300.0, 300.0)))
    [
      SNew(SBorder)
      .BorderImage(InArgs._CircleBrush)
    ]
    + SConstraintCanvas::Slot()
      .Offset(
        MakeAttributeLambda([this]
        {
          FMargin initialMargin = FMargin(0.0, 0.0, 300.0, 300.0) * (1 - (singleSubmitHandler->pivot / singleSubmitHandler-> totalLength));
          return initialMargin + FMargin(viewportPosition.X, viewportPosition.Y, 0, 0);
        }))
    [
      SNew(SBorder)
      .BorderImage(InArgs._CircleBrush)
    ]
  ];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
