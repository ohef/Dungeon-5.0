#include "SingleSubmitHandler.h"

#include "DungeonConstants.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/Accumulate.h"
#include "Algo/Transform.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
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

void USingleSubmitHandler::RemoveAfterAnimationFinished()
{
  HandlerWidget->Stop();
  interactionEnd.Execute(results);
}

void USingleSubmitHandler::BeginPlay()
{
  Super::BeginPlay();

  HandlerWidget = NewObject<UDungeonSubmitHandlerWidget>(this, HandlerWidgetClass);
  auto InPlayerController = Cast<ADungeonPlayerController>(this->GetWorld()->GetFirstPlayerController());
  HandlerWidget->SetPlayerContext(FLocalPlayerContext(InPlayerController));
  HandlerWidget->Initialize();
  HandlerWidget->AddToViewport();

  auto widget = FWidgetAnimationDynamicEvent();
  widget.BindDynamic(this, &USingleSubmitHandler::RemoveAfterAnimationFinished);
  HandlerWidget->BindToAnimationFinished(HandlerWidget->OuterDissappear, MoveTemp(widget));
  HandlerWidget->SetVisibility(ESlateVisibility::Collapsed);

  this->SetComponentTickEnabled(false);
}

void USingleSubmitHandler::Begin(TDelegate<void(TOptional<FInteractionResults>)> interactionEndd)
{
  focusWorldLocation = TilePositionToWorldPoint({2, 2});

  // TODO: allow for updating this?
  // totalLength = 3.;
  // pivot = 1.5;
  // fallOffsFromPivot = {0.3f};
  
  int orderi = 1;
  handlers.Empty();
  Algo::Transform(fallOffsFromPivot, handlers, [this, &orderi](float x)
  {
    return FIntervalPriority(pivot - x, pivot + x, orderi++);
  });
  
  timeline.SetTimelineLengthMode(ETimelineLengthMode::TL_TimelineLength);
  timeline.SetTimelineLength(totalLength);
  timeline.SetLooping(true);
  timeline.Play();

  HandlerWidget->RenderProperties(handlers, timeline.GetTimelineLength(), timeline.GetPlaybackPosition());
  HandlerWidget->SetVisibility(ESlateVisibility::Visible);
  
  interactionEnd = interactionEndd;
  this->SetComponentTickEnabled(true);
}

void USingleSubmitHandler::DoSubmit()
{
  GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, FString::SanitizeFloat(timeline.GetPlaybackPosition()), true,
                                   FVector2D::UnitVector * 3.0);
  TOptional<FIntervalPriority> found;
  for (int i = 0; i < handlers.Num(); i++)
  {
    found = handlers[i].Contains(timeline.GetPlaybackPosition()) ? TOptional(handlers[i]) : found;
  }

  if (found.IsSet())
  {
    FIntervalPriority foundResult = *found;
    this->SetComponentTickEnabled(false);
    foundResult.hitTime = timeline.GetPlaybackPosition();
    results.Add(MoveTemp(foundResult));
    HandlerWidget->HandleHit();
  }
}

void USingleSubmitHandler::TickOuterCircle(float DeltaTime)
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
  HandlerWidget->PlaybackPosition = timeline.GetPlaybackPosition();
  HandlerWidget->SetRenderTranslation(FVector2D(ScreenPosition));

  CastChecked<UCanvasPanelSlot>(HandlerWidget->OuterCircle->Slot)->SetSize(
    HandlerWidget->InitialOuterCircleSize * (1.0 - (HandlerWidget->PlaybackPosition / HandlerWidget->TimelineLength)));
}

void USingleSubmitHandler::TickComponent(float DeltaTime, ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
  TickOuterCircle(DeltaTime);
}