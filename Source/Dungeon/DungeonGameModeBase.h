// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <Engine/Classes/Components/TimelineComponent.h>
#include <Logic/DungeonGameState.h>

#include "CoreMinimal.h"
#include "SingleSubmitHandler.h"
#include "TargetsAvailableId.h"
#include "Actions/CombatAction.h"
#include "Actor/MapCursorPawn.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Curves/CurveVector.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "lager/store.hpp"
#include "State/State.h"
#include "Widget/DungeonMainWidget.h"
#include "Widget/Menus/EasyMenu.h"

#include "DungeonGameModeBase.generated.h"

class UTileVisualizationComponent;
struct FSelectingGameState;
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAttackDelegate, FAttackResults const &, AttackResults);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCombatActionEvent, FCombatAction, action);

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
  return managedPointer.IsValid() ? FCoerceToFText::Value(managedPointer->fieldName) : FText(); \
}

UCLASS()
class DUNGEON_API ADungeonGameModeBase : public AGameModeBase //, public TSharedFromThis<ADungeonGameModeBase>
{
  GENERATED_BODY()

  ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer);

public:
  virtual void BeginPlay() override;
  virtual void Tick(float time) override;
  void Dispatch(TDungeonAction&& unionAction);

  CREATE_GETTER_FOR_PROPERTY(LastSeenUnitUnderCursor, Id)
  CREATE_GETTER_FOR_PROPERTY(LastSeenUnitUnderCursor, Movement)
  CREATE_GETTER_FOR_PROPERTY(LastSeenUnitUnderCursor, Name)
  CREATE_GETTER_FOR_PROPERTY(LastSeenUnitUnderCursor, HitPoints)
  DECLARE_MULTICAST_DELEGATE_OneParam(TDungeonActionDispatched, const TDungeonAction& )
  TDungeonActionDispatched DungeonActionDispatched;

  FText GetCurrentTurnId() const;

  UPROPERTY(EditAnywhere)
  TSubclassOf<UUserWidget> MenuClass;
  UPROPERTY(EditAnywhere)
  UClass* TileShowPrefab;
  UPROPERTY(EditAnywhere)
  FDungeonWorldState Game;
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
  UTileVisualizationComponent* TileVisualizationComponent;
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  UTileVisualizationComponent* MovementVisualization;

  UPROPERTY(VisibleAnywhere)
  USingleSubmitHandler* SingleSubmitHandler;

  TUniquePtr<FDungeonLogicUnit> LastSeenUnitUnderCursor;
  FTextBlockStyle style;
  TSharedPtr<FState> baseState;
  TArray<TSharedPtr<FState>> stateStack;
  TSubclassOf<UDungeonMainWidget> MainWidgetClass;
  TQueue<TUniquePtr<FTimeline>> AnimationQueue;
  // using TStoreAction = THistoryAction<TAction>;
  // using TModelType = FHistoryModel<FDungeonWorldState>;
  TUniquePtr<TDungeonStore> store;
  lager::reader<TInteractionContext> interactionReader;

  void GoBackOnInputState();

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

  TWeakPtr<FState> GetCurrentState();

  void CommitAndGotoBaseState();

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

    onUpdate.BindLambda([movable](FVector interp)
    {
      if constexpr (TIsDerivedFrom<T, AActor>::Value)
        movable->SetActorLocation(interp, false);
      else
        movable->SetWorldLocation(interp, false);
    });

    onFinish.BindLambda([this]()
    {
      this->AnimationQueue.Pop();
    });

    timeline->SetTimelineFinishedFunc(onFinish);
    timeline->AddInterpVector(curve, onUpdate);
    timeline->Play();

    this->AnimationQueue.Enqueue(MoveTemp(timeline));
  }

  AMapCursorPawn* GetMapCursorPawn();
};
