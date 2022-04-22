#include "SingleSubmitHandler.h"

#include "DungeonSubmitHandlerWidget.h"
#include "Actor/MapCursorPawn.h"
#include "SSingleSubmitHandlerWidget.h"
#include "Algo/Transform.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"

float USingleSubmitHandler::GetCurrentTimeInTimeline()
{
  return timeline.GetPlaybackPosition();
}

USingleSubmitHandler::USingleSubmitHandler(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  static ConstructorHelpers::FObjectFinder<UMaterial> NewMaterial(
    TEXT("Material'/Game/Blueprints/NewMaterial.NewMaterial'"));
  static ConstructorHelpers::FClassFinder<UDungeonSubmitHandlerWidget> HandlerClass(
    TEXT("/Game/Blueprints/SubmitHandlerBlueprint"));

  materialBrush.SetResourceObject(NewMaterial.Object);

  HandlerWidgetClass = HandlerClass.Class;

  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USingleSubmitHandler::BeginPlay()
{
  Super::BeginPlay();

  HandlerWidget = NewObject<UDungeonSubmitHandlerWidget>(this, HandlerWidgetClass);
  APlayerController* InPlayerController = this->GetWorld()->GetFirstPlayerController();
  Cast<AMapCursorPawn>(InPlayerController->GetPawn())->QueryInput.AddUObject(this, &USingleSubmitHandler::DoSubmit);
  HandlerWidget->SetPlayerContext(FLocalPlayerContext(InPlayerController));
  HandlerWidget->singleSubmitHandler = this;
  HandlerWidget->Initialize();

  TArray<FFloatInterval> intervals;
  Algo::Transform(fallOffsFromPivot, handlers, [this](float x)
  {
    return FFloatInterval(pivot - x, pivot + x);
  });

  FVector2D position;
  UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
    InPlayerController, this->GetOwner()->GetActorLocation(),
    position, true);
  CastChecked<UCanvasPanelSlot>(HandlerWidget->OuterCircle->Slot)->SetSize(position);
  HandlerWidget->AddToViewport();

  timeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
  timeline.SetTimelineLength(totalLength);
  timeline.SetLooping(true);
  timeline.Play();
}

void USingleSubmitHandler::DoSubmit(FIntPoint)
{
  bQueryCalled = true;
}

void USingleSubmitHandler::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  timeline.TickTimeline(DeltaTime);

  FFloatInterval* found = nullptr;
  for (int i = 0; i < handlers.Num(); i++)
  {
    found = handlers[i].Contains(timeline.GetPlaybackPosition()) ? handlers.GetData() + i : nullptr;
  }

  if (found != nullptr && bQueryCalled)
  {
    IntervalHit.Broadcast();
    bQueryCalled = false;
    this->SetComponentTickEnabled(false);
  }
}
