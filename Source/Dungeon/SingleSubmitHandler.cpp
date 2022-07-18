#include "SingleSubmitHandler.h"

#include "DungeonSubmitHandlerWidget.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/Transform.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

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

float USingleSubmitHandler::GetCurrentTimeInTimeline()
{
  return timeline.GetPlaybackPosition();
}

void USingleSubmitHandler::EndInteraction(){
  HandlerWidget->RemoveFromViewport();
  DestroyComponent();
}

void USingleSubmitHandler::RemoveAfterAnimationFinished()
{
  InteractionFinished.Broadcast(results);
  EndInteraction();
}

void USingleSubmitHandler::BeginPlay()
{
  Super::BeginPlay();

  HandlerWidget = NewObject<UDungeonSubmitHandlerWidget>(this, HandlerWidgetClass);
  auto InPlayerController = Cast<ADungeonPlayerController>(this->GetWorld()->GetFirstPlayerController());
  FQueryInput& QueryInput = InPlayerController->QueryInput;
  FDelegateHandle handle = QueryInput.AddUObject(this, &USingleSubmitHandler::DoSubmit);

  stopCheckingQueries = [handle, &QueryInput]()
  {
    if (handle.IsValid())
    {
      QueryInput.Remove(handle);
    }
  };

  HandlerWidget->SetPlayerContext(FLocalPlayerContext(InPlayerController));
  HandlerWidget->singleSubmitHandler = this;
  HandlerWidget->Initialize();

  TArray<FIntervalPriority> intervals;
  int orderi = 1;
  Algo::Transform(fallOffsFromPivot, handlers, [this, &orderi](float x)
  {
    return FIntervalPriority(pivot - x, pivot + x, orderi++);
  });
  HandlerWidget->AddToViewport();

  auto widget = FWidgetAnimationDynamicEvent();
  widget.BindDynamic(this, &USingleSubmitHandler::RemoveAfterAnimationFinished);
  HandlerWidget->BindToAnimationFinished(HandlerWidget->OuterDissappear, MoveTemp(widget));

  timeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
  timeline.SetTimelineLength(totalLength);
  timeline.SetLooping(true);
  timeline.Play();
}

void USingleSubmitHandler::DoSubmit(FIntPoint)
{
  FIntervalPriority* found = nullptr;
  for (int i = 0; i < handlers.Num(); i++)
  {
    found = handlers[i].Contains(timeline.GetPlaybackPosition()) ? handlers.GetData() + i : nullptr;
  }

  if (found != nullptr)
  {
    auto foundResult = *found;
    foundResult.hitTime = timeline.GetPlaybackPosition();
    results.Add(MoveTemp(foundResult));
    this->SetComponentTickEnabled(false);
    timeline.Stop();
    HandlerWidget->HandleHit();
    stopCheckingQueries();
  }
}

void USingleSubmitHandler::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  timeline.TickTimeline(DeltaTime);

  APlayerController* InPlayerController = this->GetWorld()->GetFirstPlayerController();
  FVector2D ScreenPosition;
  UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(InPlayerController, focusWorldLocation, ScreenPosition,
                                                             false);
  HandlerWidget->SetRenderTranslation(FVector2D(ScreenPosition));
}
