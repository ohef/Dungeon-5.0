// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include <Engine/Classes/Components/TimelineComponent.h>
#include <Logic/DungeonGameState.h>

#include "DungeonMainWidget.h"
#include "SingleSubmitHandler.h"
#include "Actions/Action.h"
#include "Actions/CombatAction.h"
#include "Actions/MoveAction.h"
#include "Actor/MapCursorPawn.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/TileVisualizationComponent.h"
#include "Components/VerticalBoxSlot.h"
#include "Curves/CurveVector.h"
#include "Engine/DataTable.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widget/Menus/EasyMenu.h"

#include "DungeonGameModeBase.generated.h"

struct ProgramState;
USTRUCT(BlueprintType)
struct FAbilityParams : public FTableRowBase
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon)
  UMaterialInstance* coloring;
};

USTRUCT()
struct FAttackResults
{
  GENERATED_BODY()

  TSet<FIntPoint> AllAttackTiles;
  TSet<FIntPoint> AttackableUnitTiles;
};

USTRUCT()
struct FStructThing : public FTableRowBase
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  int ID;
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  int HP;
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  int Movement;
};

USTRUCT()
struct FTiledLayer
{
  GENERATED_BODY()

  UPROPERTY()
  TArray<int> data;
  UPROPERTY()
  int height;
  UPROPERTY()
  int width;
};

USTRUCT()
struct FTiledMap
{
  GENERATED_BODY()

  UPROPERTY()
  TArray<FTiledLayer> layers;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttackDelegate, FAttackResults const &, AttackResults);

class ADungeonGameModeBase;

struct FState
{
  virtual ~FState() = default;

  virtual void Enter()
  {
  };

  virtual void Exit()
  {
  };
};

struct FStateHook : FState
{
  TFunction<TFunction<void()>()> startingHook;
  TFunction<void()> forExit;

  explicit FStateHook(const TFunction<TFunction<void()>()>& StartingHook)
    : startingHook(StartingHook)
  {
  }

  virtual void Enter()
  {
    forExit = startingHook();
  };

  virtual void Exit()
  {
    forExit();
  };
};

UENUM()
enum ETargetsAvailableId
{
  move,
  attack
};

struct FSelectingGameState : public FState
{
  ADungeonGameModeBase& gameMode;
  FDungeonLogicUnit* foundUnit;
  TMap<ETargetsAvailableId, TSet<FIntPoint>> targets;
  TFunction<void()> unregisterDelegates;
  TSharedPtr<FStateHook> hook;

  explicit FSelectingGameState(ADungeonGameModeBase& GameMode)
    : gameMode(GameMode), foundUnit(nullptr)
  {
    targets.Add(ETargetsAvailableId::move, TSet<FIntPoint>());
    targets.Add(ETargetsAvailableId::attack, TSet<FIntPoint>());
  }

  void OnCursorPositionUpdate(FIntPoint);
  void OnCursorQuery(FIntPoint);

  virtual void Enter() override;
  virtual void Exit() override;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCombatActionEvent, FCombatAction, action);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUnitUpdate, FDungeonLogicUnit, unit);

struct FCoerceToFText
{
  template <typename T>
  static typename TEnableIf<TIsArithmetic<T>::Value, FText>::Type Value(T thing)
  {
    return FText::AsNumber(thing);
  }

  static FText Value(FString thing)
  {
    return FText::FromString(thing);
  }
};

#define CREATE_GETTER_FOR_PROPERTY(managedPointer, fieldName) \
FText Get##managedPointer####fieldName() const \
{ \
  return managedPointer == nullptr ? FText() : FCoerceToFText::Value(managedPointer->fieldName); \
}

using TAction = TVariant<FEmptyVariantState, FMoveAction, FCombatAction, FWaitAction>;

UCLASS()
class DUNGEON_API ADungeonGameModeBase : public AGameModeBase, public TSharedFromThis<ADungeonGameModeBase>
{
  GENERATED_BODY()

  ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer);

public:
  virtual void BeginPlay() override;
  virtual void Tick(float time) override;
  void UpdateUnitActor(const FDungeonLogicUnit& unit);
  void Reduce(TAction unionAction);

  CREATE_GETTER_FOR_PROPERTY(LastSeenUnitUnderCursor, Id)
  CREATE_GETTER_FOR_PROPERTY(LastSeenUnitUnderCursor, Movement)
  CREATE_GETTER_FOR_PROPERTY(LastSeenUnitUnderCursor, Name)
  CREATE_GETTER_FOR_PROPERTY(LastSeenUnitUnderCursor, HitPoints)

  auto GetCurrentTurnId() const
  {
    return FCoerceToFText::Value(Game->turnState.TeamId);
  }

  void Hello(ProgramState helloDaddy);
  UPROPERTY(EditAnywhere)
  TSubclassOf<UUserWidget> MenuClass;
  UPROPERTY(EditAnywhere)
  UClass* TileShowPrefab;
  UPROPERTY(EditAnywhere)
  UDungeonLogicGameState* Game;
  UPROPERTY(EditAnywhere)
  UClass* UnitActorPrefab;
  UPROPERTY(EditAnywhere)
  UDataTable* UnitTable;
  UPROPERTY(EditAnywhere)
  TMap<TEnumAsByte<ETargetsAvailableId>, FAbilityParams> TargetsColoring;
  UPROPERTY(BlueprintAssignable)
  FCombatActionEvent CombatActionEvent;
  UPROPERTY()
  UDungeonMainWidget* MainWidget;
  UPROPERTY()
  UTileVisualizationComponent* tileVisualizationComponent;

  FDungeonLogicUnit* LastSeenUnitUnderCursor;
  FTextBlockStyle style;
  TSharedPtr<FSelectingGameState> baseState;
  TArray<TSharedPtr<FState>> stateStack;
  TSubclassOf<UDungeonMainWidget> MainWidgetClass;
  TQueue<TUniquePtr<FTimeline>> AnimationQueue;
  TQueue<TTuple<TUniquePtr<FAction>, TFunction<void()>>> reactEventQueue;

  void GoBackOnInputState()
  {
    if (stateStack.IsEmpty())
      return;
    auto popped = stateStack.Pop();
    popped->Exit();

    if (stateStack.IsEmpty())
      baseState->Enter();
    else
      stateStack.Top()->Enter();
  }

  template <typename TState>
  void InputStateTransition(TState* state)
  {
    if (stateStack.IsEmpty())
      baseState->Exit();
    else
      stateStack.Top()->Exit();

    stateStack.Push(MakeShareable<TState>(state));
    stateStack.Top()->Enter();
  }

  TWeakPtr<FState> GetCurrentState()
  {
    return stateStack.IsEmpty() ? baseState : stateStack.Top();
  }

  void CommitAndGotoBaseState()
  {
    GetCurrentState().Pin()->Exit();
    stateStack.Empty();
    GetCurrentState().Pin()->Enter();
  }

  void React();

  FAttackResults AttackVisualization(int attackingUnitId);

  TOptional<FDungeonLogicUnit> FindUnit(FIntPoint pt);

  UFUNCTION()
  void ApplyAction(FCombatAction action);

  void RefocusMenu();

  template <typename T>
  void SubmitLinearAnimation(T* movable, FIntPoint from, FIntPoint to, float time)
  {
    auto timeline = MakeUnique<FTimeline>();
    FOnTimelineVectorStatic onUpdate;
    FOnTimelineEventStatic onFinish;
    UCurveVector* curve = NewObject<UCurveVector>();
    from = from * 100;
    to = to * 100;
    curve->FloatCurves[0].AddKey(0.0, from.X);
    curve->FloatCurves[0].AddKey(time, to.X);
    curve->FloatCurves[1].AddKey(0.0, from.Y);
    curve->FloatCurves[1].AddKey(time, to.Y);

    if constexpr (TIsDerivedFrom<T, AActor>::Value)
    {
      onUpdate.BindLambda([movable](FVector interp)
      {
        movable->SetActorLocation(interp, false);
      });
    }
    else
    {
      onUpdate.BindLambda([movable](FVector interp)
      {
        movable->SetWorldLocation(interp, false);
      });
    }

    onFinish.BindLambda([this]()
    {
      this->AnimationQueue.Pop();
    });

    timeline->SetTimelineFinishedFunc(onFinish);
    timeline->AddInterpVector(curve, onUpdate);
    timeline->Play();

    this->AnimationQueue.Enqueue(MoveTemp(timeline));
  }

  AMapCursorPawn* GetMapCursorPawn()
  {
    return this
           ->GetWorld()
           ->GetFirstPlayerController()
           ->GetPawn<AMapCursorPawn>();
  }

  bool canUnitMoveToPointInRange(int unitId, FIntPoint destination, const TSet<FIntPoint>& movementExtent)
  {
    auto& map = Game->map;
    if (map.unitAssignment.Contains(destination))
      return false;

    if (map.unitAssignment.FindKey(unitId) == nullptr)
      return false;

    return movementExtent.Contains(destination) && Game->unitIdToActor.Contains(unitId);
  }
};

struct FAttackState : public FState
{
  ADungeonGameModeBase& gameMode;
  TWeakObjectPtr<USingleSubmitHandler> SingleSubmitHandler;

  FAttackState(ADungeonGameModeBase& GameMode, USingleSubmitHandler* SingleSubmitHandler)
    : gameMode(GameMode),
      SingleSubmitHandler(SingleSubmitHandler)
  {
  }

  virtual void Enter() override
  {
    // gameMode.GetMapCursorPawn()->DisableInput(gameMode.GetWorld()->GetFirstPlayerController());
    Invoke(&AMapCursorPawn::DisableInput, *gameMode.GetMapCursorPawn(),
           gameMode.GetWorld()->GetFirstPlayerController());
  }

  virtual void Exit() override
  {
    Invoke(&AMapCursorPawn::EnableInput, *gameMode.GetMapCursorPawn(), gameMode.GetWorld()->GetFirstPlayerController());
    // gameMode.GetMapCursorPawn()->EnableInput(gameMode.GetWorld()->GetFirstPlayerController());
    if (SingleSubmitHandler.IsValid())
    {
      SingleSubmitHandler->EndInteraction();
    }
  }
};

struct FSelectingTargetGameState : public FState
{
  ADungeonGameModeBase& gameMode;
  TFunction<void(FIntPoint)> handleQuery;
  FDelegateHandle DelegateHandle;

  FSelectingTargetGameState(ADungeonGameModeBase& GameModeBase,
                            TFunction<void(FIntPoint)>&& QueryHandlerFunction
  ) : gameMode(GameModeBase),
      handleQuery(QueryHandlerFunction)
  {
  }

  virtual void Enter() override
  {
    DelegateHandle = gameMode.GetMapCursorPawn()->QueryInput.AddLambda(handleQuery);
  }

  virtual void Exit() override
  {
    gameMode.GetMapCursorPawn()->QueryInput.Remove(DelegateHandle);
  }
};

struct FSelectingMenu : public FState, TSharedFromThis<FSelectingMenu>
{
  ADungeonGameModeBase& gameMode;
  FIntPoint capturedCursorPosition;
  FDungeonLogicUnit* initiatingUnit;
  TMap<ETargetsAvailableId, TSet<FIntPoint>> targets;

  FSelectingMenu(ADungeonGameModeBase& GameMode, const FIntPoint& CapturedCursorPosition,
                 FDungeonLogicUnit* InitiatingUnit, const TMap<ETargetsAvailableId, TSet<FIntPoint>>& Targets)
    : gameMode(GameMode),
      capturedCursorPosition(CapturedCursorPosition),
      initiatingUnit(InitiatingUnit),
      targets(Targets)
  {
  }

  virtual void Enter() override
  {
    AMapCursorPawn* MapCursorPawn = gameMode.GetMapCursorPawn();

    if (MapCursorPawn->CurrentPosition != capturedCursorPosition)
    {
      // UInterpToMovementComponent* InterpToMovementComponent = MapCursorPawn->InterpToMovementComponent;
      // InterpToMovementComponent->ResetControlPoints();
      // InterpToMovementComponent->AddControlPointPosition(TilePositionToWorldPoint(MapCursorPawn->CurrentPosition), false);
      // InterpToMovementComponent->AddControlPointPosition(TilePositionToWorldPoint(capturedCursorPosition), false);
      // InterpToMovementComponent->FinaliseControlPoints();
      // InterpToMovementComponent->RestartMovement();
      MapCursorPawn->SetActorLocation(TilePositionToWorldPoint(capturedCursorPosition));
    }

    MapCursorPawn->MovementComponent->ToggleActive();
    gameMode.MainWidget->MainMenu->SetVisibility(ESlateVisibility::Visible);
    gameMode.RefocusMenu();

    // gameMode.MainWidget->MainMenu->AddChildToVerticalBox()
    auto easyMenu = gameMode.MainWidget->WidgetTree->ConstructWidget<UEasyMenu>();

    // UButton* Button = gameMode.MainWidget->WidgetTree->ConstructWidget<UButton>();
    // UTextBlock* textWidget = gameMode.MainWidget->WidgetTree->ConstructWidget<UTextBlock>();
    // textWidget->Text = FText::FromString( "so it actually works");
    // Button->AddChild(textWidget);
    // gameMode.MainWidget->MainMenu->AddChildToVerticalBox(Button);
    gameMode.MainWidget->MainMenu->AddChildToVerticalBox(easyMenu);
    UEasyMenuEntry* EasyMenuEntry = NewObject<UEasyMenuEntry>(easyMenu);
    EasyMenuEntry->Label = "PrintsCheckThisShitOutDude";
    EasyMenuEntry->handler = [&]() { UKismetSystemLibrary::PrintString(&gameMode, "dude check this shit out dude"); };
    UVerticalBoxSlot* VerticalBoxSlot = easyMenu->AddEntry(MoveTemp(EasyMenuEntry));
    VerticalBoxSlot->SetPadding({5.0,5.0});
    // easyMenu->AddEntry("dude");
    // easyMenu->AddEntry("holyshit");
    // Button->SetStyle()

    //TODO: eh could template this somehow
    StaticCastSharedRef<SButton>(gameMode.MainWidget->Move->TakeWidget())
      ->SetOnClicked(FOnClicked::CreateSP(this->AsShared(), &FSelectingMenu::OnMoveSelected));

    StaticCastSharedRef<SButton>(gameMode.MainWidget->Attack->TakeWidget())
      ->SetOnClicked(FOnClicked::CreateSP(this->AsShared(), &FSelectingMenu::OnAttackButtonClick));

    StaticCastSharedRef<SButton>(gameMode.MainWidget->Wait->TakeWidget())
      ->SetOnClicked(FOnClicked::CreateSP(this->AsShared(), &FSelectingMenu::OnWaitButtonClick));
  }

  virtual void Exit() override
  {
    AMapCursorPawn* MapCursorPawn = gameMode.GetMapCursorPawn();
    MapCursorPawn->MovementComponent->ToggleActive();
    gameMode.MainWidget->MainMenu->SetVisibility(ESlateVisibility::Collapsed);
  }

  FReply OnAttackButtonClick();

  FReply OnWaitButtonClick();

  FReply OnMoveSelected();
};

using TState = TVariant<FEmptyVariantState, FAttackState, FSelectingMenu, FSelectingTargetGameState>;

struct ProgramState
{
  TState state;
};

template <class... Ts>
struct StateHandler : Ts...
{
  using Ts::operator()...;
};

template <class... Ts>
StateHandler(Ts ...) -> StateHandler<Ts...>;

inline void ADungeonGameModeBase::Hello(ProgramState helloDaddy)
{
  Visit(StateHandler{
          [this](FAttackState) -> TFunction<void()>
          {
            this->GetMapCursorPawn()->DisableInput(this->GetWorld()->GetFirstPlayerController());
            return [&]()
            {
              this->GetMapCursorPawn()->EnableInput(this->GetWorld()->GetFirstPlayerController());
            };
          },
          [](FSelectingMenu) -> TFunction<void()>
          {
          },
          [](FSelectingTargetGameState) -> TFunction<void()>
          {
          },
          [](auto) -> TFunction<void()>
          {
          }
        }, helloDaddy.state);
}
