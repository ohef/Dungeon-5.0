#include "SingleSubmitHandler.h"

#include "DungeonSubmitHandlerWidget.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/Transform.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/KismetSystemLibrary.h"

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
  FQueryInput QueryInput = Cast<AMapCursorPawn>(InPlayerController->GetPawn())->QueryInput;
  FDelegateHandle handle = QueryInput.AddUObject(this, &USingleSubmitHandler::DoSubmit);

  stopCheckingQueries = [handle, QueryInput]() mutable
  {
    if (handle.IsValid())
    {
      QueryInput.Remove(handle);
      handle.Reset();
    }
  };

  HandlerWidget->SetPlayerContext(FLocalPlayerContext(InPlayerController));
  HandlerWidget->singleSubmitHandler = this;
  HandlerWidget->Initialize();

  TArray<FFloatInterval> intervals;
  Algo::Transform(fallOffsFromPivot, handlers, [this](float x)
  {
    return FFloatInterval(pivot - x, pivot + x);
  });

  FVector2D position;
  FVector focusWorldLocation = this->GetOwner()->GetActorLocation();
  UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
    InPlayerController, focusWorldLocation,
    position, true);
  CastChecked<UCanvasPanelSlot>(HandlerWidget->OuterCircle->Slot)->SetSize(position);
  HandlerWidget->AddToViewport();

  timeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
  timeline.SetTimelineLength(totalLength);
  timeline.SetLooping(true);
  timeline.Play();
}

void USingleSubmitHandler::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
  Super::PostEditChangeProperty(PropertyChangedEvent);
}

void USingleSubmitHandler::DoSubmit(FIntPoint)
{
  FFloatInterval* found = nullptr;
  for (int i = 0; i < handlers.Num(); i++)
  {
    found = handlers[i].Contains(timeline.GetPlaybackPosition()) ? handlers.GetData() + i : nullptr;
  }

  if (found != nullptr)
  {
    IntervalHit.Broadcast();
    this->SetComponentTickEnabled(false);
    stopCheckingQueries();
  }
}

void USingleSubmitHandler::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  timeline.TickTimeline(DeltaTime);
}
