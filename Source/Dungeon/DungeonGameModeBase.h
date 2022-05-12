// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include <Engine/Classes/Components/TimelineComponent.h>
#include <Logic/game.h>

#include "DungeonMainWidget.h"
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

UCLASS()
class DUNGEON_API ADungeonGameModeBase : public AGameModeBase
{
  GENERATED_BODY()

  ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer);

public:
  virtual void BeginPlay() override;
  virtual void Tick(float time) override;

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

  TArray<FDungeonLogicUnit*> unitsArray;

  UPROPERTY()
  UDungeonMainWidget* MainWidget;
  UPROPERTY()
  UTileVisualizationComponent* tileVisualizationComponent;

  FAttackDelegate AttackDelegate;

  TSubclassOf<UDungeonMainWidget> MainWidgetClass;

  TQueue<TUniquePtr<FTimeline>> AnimationQueue;
  TSet<FIntPoint> lastMoveLocations;
  int lastSeenId;

  ReactStateVariable(EGameState, CurrentGameState)
  ReactStateVariable(FIntPoint, CurrentPosition)

  TQueue<TSharedPtr<FAction>> reactEventQueue;

  void ReactMenu(bool visible);
  void ReactVisualization(TSet<FIntPoint> moveLocations, EGameState PassedGameState);
  void React();
  void ReactTargeting();

  FAttackResults AttackVisualization(int attackingUnitId);

  UFUNCTION()
  void HandleMove();
  UFUNCTION()
  void HandleWait();

  template <typename TAction>
  void Dispatch(TAction&& action)
  {
    reactEventQueue.Enqueue(MakeShared<TAction>(MoveTemp(action)));
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
};

struct FAction
{
  virtual ~FAction() = default;

  virtual void Apply(ADungeonGameModeBase*)
  {
  };
};

struct FSelectingGameState
{
  ADungeonGameModeBase& gameMode;
  
  void OnCursorPositionUpdate();
  void Update();
};

struct FSelectingTargetGameState
{
  FDungeonLogicUnit* instigator;
  UMaterial* tileMaterial;
  TArray<FIntPoint> TilesExtent;
  TArray<FIntPoint> PossibleTargets;

  void Update();
  void ApplyToTarget(FIntPoint point);
};

struct FChangeState : FAction
{
  explicit FChangeState(EGameState UpdatedState)
    : updatedState(UpdatedState)
  {
  }

  EGameState updatedState;

  virtual void Apply(ADungeonGameModeBase* map) override
  {
    map->setCurrentGameState(updatedState);
  }
};

struct FCombatAction : FAction
{
  FCombatAction(int InitiatorId, int TargetId)
    : InitiatorId(InitiatorId),
      TargetId(TargetId)
  {
  }

  int InitiatorId;
  int TargetId;

  virtual void Apply(ADungeonGameModeBase* modeBase) override
  {
    FDungeonLogicUnit& initiator = modeBase->Game->map.loadedUnits.FindChecked(InitiatorId);
    FDungeonLogicUnit& target = modeBase->Game->map.loadedUnits.FindChecked(TargetId);
    target.hitPoints -= initiator.damage;
  }
};

struct FMoveAction : FAction
{
  FMoveAction(int ID, const FIntPoint& Destination)
    : id(ID),
      Destination(Destination)
  {
  }

  int id;
  FIntPoint Destination;

  bool canMove(ADungeonGameModeBase* gameModeBase, const TSet<FIntPoint>& lastMoveLocations)
  {
    auto& map = gameModeBase->Game->map;
    auto possibleLocation = map.unitAssignment.FindKey(id);
    if (possibleLocation == nullptr) return false;

    if (lastMoveLocations.Contains(Destination))
    {
      auto unit = gameModeBase->Game->unitIdToActor.Find(id);
      if (unit == nullptr)
        return false;

      return true;
    }

    return false;
  }

  virtual void Apply(ADungeonGameModeBase* modeBase) override
  {
    FDungeonLogicMap& DungeonLogicMap = modeBase->Game->map;
    DungeonLogicMap.unitAssignment.Remove(*DungeonLogicMap.unitAssignment.FindKey(id));
    DungeonLogicMap.unitAssignment.Add(Destination, id);
    auto& foundUnit = DungeonLogicMap.loadedUnits.FindChecked(id);
    foundUnit.state = UnitState::ActionTaken;
  };
};
