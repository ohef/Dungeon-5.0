#include "SingleSubmitHandler.h"

#include "DungeonSubmitHandlerWidget.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/Accumulate.h"
#include "Algo/Transform.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"

USingleSubmitHandler::USingleSubmitHandler(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
  static ConstructorHelpers::FObjectFinder<UMaterial> NewMaterial(
    TEXT("Material'/Game/Blueprints/NewMaterial.NewMaterial'"));
  static ConstructorHelpers::FClassFinder<UDungeonSubmitHandlerWidget> HandlerClass(
    TEXT("/Game/Blueprints/Widgets/SubmitHandlerBlueprint"));

  materialBrush.SetResourceObject(NewMaterial.Object);
  HandlerWidgetClass = HandlerClass.Class;
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.bStartWithTickEnabled = true;
}

void USingleSubmitHandler::EndInteraction()
{
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

  int orderi = 1;
  Algo::Transform(fallOffsFromPivot, handlers, [this, &orderi](float x)
  {
    return FIntervalPriority(pivot - x, pivot + x, orderi++);
  });

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
  GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::SanitizeFloat(timeline.GetPlaybackPosition()), true,
                                   FVector2D::UnitVector * 3.0);
  FIntervalPriority* found = nullptr;
  for (int i = 0; i < handlers.Num(); i++)
  {
    found = handlers[i].Contains(timeline.GetPlaybackPosition()) ? handlers.GetData() + i : nullptr;
  }

  if (found != nullptr)
  {
    auto foundResult = *found;
    this->SetComponentTickEnabled(false);
    timeline.Stop();
    foundResult.hitTime = timeline.GetPlaybackPosition();
    results.Add(MoveTemp(foundResult));
    HandlerWidget->HandleHit();
    stopCheckingQueries();
  }
}

void USingleSubmitHandler::DoDaTick(float DeltaTime)
{
  timeline.TickTimeline(DeltaTime);

  auto output =
    Algo::Accumulate(handlers, FString{}, [](FString acc, decltype(handlers)::ElementType val)
    {
      return acc.Append(FString::Format(TEXT("Min {0}, Max {1}, Priority {2}\n"), {val.Min, val.Max, val.order}));
    });

  APlayerController* InPlayerController = this->GetWorld()->GetFirstPlayerController();
  FVector2D ScreenPosition;
  UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(InPlayerController, focusWorldLocation, ScreenPosition,
                                                             false);
  HandlerWidget->SetRenderTranslation(FVector2D(ScreenPosition));
}

void USingleSubmitHandler::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  DoDaTick(DeltaTime);
}
