// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include <Engine/Classes/Components/TimelineComponent.h>
#include <Logic/game.h>

#include "DungeonMainWidget.h"
#include "SingleSubmitHandler.h"
#include "Actor/MapCursorPawn.h"
#include "Blueprint/UserWidget.h"
#include "Components/TileVisualizationComponent.h"
#include "Curves/CurveVector.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameState.h"

#include "DungeonGameModeBase.generated.h"

#define ReactStateVariable(SymbolType, SymbolName)\
  SymbolType SymbolName;\
  void set ## SymbolName( SymbolType val )\
  {\
    this->SymbolName = val;\
    this->Reduce();\
  }

USTRUCT(BlueprintType)
struct FAbilityParams : public FTableRowBase
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Dungeon)
  UMaterialInstance* coloring;
};

struct FAction;

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

enum EGameState
{
  Selecting,
  SelectingAbility,
  SelectingTarget,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttackDelegate, FAttackResults const &, AttackResults);

class ADungeonGameModeBase;

//iUSTRUCT()<c-c>jjoGENERATED_BODY()<c-c>

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

template <typename TToClass, typename TFromClass, typename... InArgs>
static auto MakeChangeState(TFromClass* from, InArgs&&... Args) -> TSharedPtr<TToClass>
{
  return MakeShared<TToClass>(Forward<InArgs>(Args)...);
}

USTRUCT()
struct FAction
{
  GENERATED_BODY()
  virtual ~FAction() = default;
};

USTRUCT(BlueprintType)
struct FCombatAction : public FAction
{
  GENERATED_BODY()

  FCombatAction() = default;

  FCombatAction(int InitiatorId, const FDungeonLogicUnit& UpdatedUnit, double DamageValue, const FIntPoint& Target)
    : InitiatorId(InitiatorId),
      updatedUnit(UpdatedUnit),
      damageValue(DamageValue),
      target(Target)
  {
  }

  UPROPERTY(BlueprintReadWrite)
  int InitiatorId;
  UPROPERTY(BlueprintReadWrite)
  FDungeonLogicUnit updatedUnit;
  UPROPERTY(BlueprintReadWrite)
  double damageValue;
  UPROPERTY(BlueprintReadWrite)
  FIntPoint target;
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
FText get_##managedPointer##_##fieldName() const \
{ \
  return managedPointer == nullptr ? FText() : FCoerceToFText::Value(managedPointer->fieldName); \
}

UCLASS()
class DUNGEON_API ADungeonGameModeBase : public AGameModeBase, public TSharedFromThis<ADungeonGameModeBase>
{
  GENERATED_BODY()

  ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer);

public:
  virtual void BeginPlay() override;
  virtual void Tick(float time) override;
  void UpdateUnitActor(const FDungeonLogicUnit& unit);

  CREATE_GETTER_FOR_PROPERTY(lastSeenUnitUnderCursor, id)
  CREATE_GETTER_FOR_PROPERTY(lastSeenUnitUnderCursor, movement)
  CREATE_GETTER_FOR_PROPERTY(lastSeenUnitUnderCursor, name)
  CREATE_GETTER_FOR_PROPERTY(lastSeenUnitUnderCursor, hitPoints)

  UPROPERTY(EditAnywhere)
  TSubclassOf<UUserWidget> MenuClass;
  UPROPERTY(EditAnywhere)
  UClass* TileShowPrefab;
  UPROPERTY(EditAnywhere)
  UDungeonLogicGame* Game;
  UPROPERTY(EditAnywhere)
  UClass* UnitActorPrefab;
  UPROPERTY(EditAnywhere)
  UStaticMesh* UnitMeshIndicator;
  UPROPERTY(EditAnywhere)
  float MovementAnimationSpeedInSeconds;
  UPROPERTY(EditAnywhere)
  UMaterialInstance* UnitCommitMaterial;
  UPROPERTY(EditAnywhere)
  UDataTable* UnitTable;
  UPROPERTY(EditAnywhere)
  TMap<TEnumAsByte<ETargetsAvailableId>, FAbilityParams> TargetsColoring;
  UPROPERTY(EditAnywhere)
  UStaticMesh* DaMesh;

  FDungeonLogicUnit* lastSeenUnitUnderCursor;

  FTextBlockStyle style;

  UPROPERTY(BlueprintAssignable)
  FCombatActionEvent CombatActionEvent;
  UPROPERTY(BlueprintAssignable)
  FUnitUpdate UnitUpdateEvent;

  UPROPERTY()
  UDungeonMainWidget* MainWidget;
  UPROPERTY()
  UTileVisualizationComponent* tileVisualizationComponent;

  FAttackDelegate AttackDelegate;

  TSharedPtr<FSelectingGameState> baseState;
  TArray<TSharedPtr<FState>> stateStack;

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

  TWeakPtr<FState> GetCurrentState()
  {
    return stateStack.IsEmpty() ? baseState : stateStack.Top();
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

  void CommitAndGotoBaseState()
  {
    GetCurrentState().Pin()->Exit();
    stateStack.Empty();
    GetCurrentState().Pin()->Enter();
  }

  TSubclassOf<UDungeonMainWidget> MainWidgetClass;

  TQueue<TUniquePtr<FTimeline>> AnimationQueue;
  TSet<FIntPoint> lastMoveLocations;

  ReactStateVariable(EGameState, CurrentGameState)
  ReactStateVariable(FIntPoint, CurrentPosition)

  TQueue<TTuple<TUniquePtr<FAction>, TFunction<void()>>> reactEventQueue;

  void React();

  FAttackResults AttackVisualization(int attackingUnitId);

  TOptional<FDungeonLogicUnit> FindUnit(FIntPoint pt);

  UFUNCTION()
  void ApplyAction(FCombatAction action);
  UFUNCTION()
  void HandleWait();

  template <typename TAction>
  void Dispatch(TAction* action, TFunction<void()>&& consumer)
  {
    reactEventQueue.Enqueue(TTuple<TUniquePtr<TAction>, TFunction<void()>>(TUniquePtr<TAction>(action), consumer));
    Reduce();
  }

  void Reduce();
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
    UWorld* InWorld = this->GetWorld();
    APlayerController* controller = InWorld->GetFirstPlayerController();
    return Cast<AMapCursorPawn>(controller->GetPawn());
  }

  bool canUnitMoveToPointInRange(int unitId, FIntPoint destination, const TSet<FIntPoint>& movementExtent)
  {
    auto& map = Game->map;
    if (map.unitAssignment.Contains(destination))
      return false;

    auto possibleLocation = map.unitAssignment.FindKey(unitId);
    if (possibleLocation == nullptr) return false;

    if (movementExtent.Contains(destination))
    {
      auto unit = Game->unitIdToActor.Find(unitId);
      if (unit == nullptr)
        return false;

      return true;
    }

    return false;
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
    gameMode.GetMapCursorPawn()->DisableInput(gameMode.GetWorld()->GetFirstPlayerController());
  }

  virtual void Exit() override
  {
    gameMode.GetMapCursorPawn()->EnableInput(gameMode.GetWorld()->GetFirstPlayerController());
    if (SingleSubmitHandler.IsValid())
    {
      SingleSubmitHandler->EndInteraction();
    }
  }
};

template <typename TTransitionState = FAttackState>
struct FSelectingTargetGameState : public FState
{
  ADungeonGameModeBase& gameMode;
  FDungeonLogicUnit& instigator;
  TSet<FIntPoint> tilesExtent;
  TFunction<void(FIntPoint)> handleQuery;

  FSelectingTargetGameState(ADungeonGameModeBase& GameModeBase, FDungeonLogicUnit& Instigator,
                            const TSet<FIntPoint>& TilesExtent,
                            TFunction<void(FIntPoint)>&& QueryHandlerFunction)
    : gameMode(GameModeBase),
      instigator(Instigator),
      tilesExtent(TilesExtent),
      handleQuery(QueryHandlerFunction)
  {
  }

  FDelegateHandle DelegateHandle;

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

    auto moveButton = StaticCastSharedRef<SButton>(gameMode.MainWidget->Move->TakeWidget());
    moveButton->SetOnClicked(FOnClicked::CreateSP(this->AsShared(), &FSelectingMenu::OnMoveSelected));

    auto attackWidget = StaticCastSharedRef<SButton>(gameMode.MainWidget->Attack->TakeWidget());
    attackWidget->SetOnClicked(FOnClicked::CreateSP(this->AsShared(), &FSelectingMenu::OnAttackButtonClick));
  }

  virtual void Exit() override
  {
    AMapCursorPawn* MapCursorPawn = gameMode.GetMapCursorPawn();
    MapCursorPawn->MovementComponent->ToggleActive();
    gameMode.MainWidget->MainMenu->SetVisibility(ESlateVisibility::Collapsed);
  }

  FReply OnAttackButtonClick();

  FReply OnMoveSelected();
};

struct FMoveAction : public FAction
{
  int id;
  FIntPoint Destination;

  FMoveAction(int ID, const FIntPoint& Destination)
    : id(ID),
      Destination(Destination)
  {
  }
};
