// Copyright Epic Games, Inc. All Rights Reserved.

#include "DungeonGameModeBase.h"

#include <Logic/DungeonGameState.h>

#include "DungeonMainWidget.h"
#include "SingleSubmitHandler.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/Accumulate.h"
#include "Algo/Copy.h"
#include "Algo/FindLast.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "lager/util.hpp"

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) :
  Super(ObjectInitializer)
{
  static ConstructorHelpers::FClassFinder<UDungeonMainWidget> HandlerClass(TEXT("/Game/Blueprints/MainWidget"));
  MainWidgetClass = HandlerClass.Class;

  PrimaryActorTick.bCanEverTick = true;
  UnitTable = ObjectInitializer.CreateDefaultSubobject<UDataTable>(this, "Default");
  UnitTable->RowStruct = FDungeonLogicUnit::StaticStruct();
  Game = ObjectInitializer.CreateDefaultSubobject<UDungeonLogicGameState>(this, "ZaGameZo");
}

void ADungeonGameModeBase::BeginPlay()
{
  Super::BeginPlay();

  TArray<FDungeonLogicUnitRow*> unitsArray;
  UnitTable->GetAllRows(TEXT(""), unitsArray);
  TMap<int, FDungeonLogicUnitRow> loadedUnitTypes;
  for (auto unit : unitsArray)
  {
    loadedUnitTypes.Add(unit->unitData.Id, *unit);
  }

  FString jsonStringTMX;
  if (FFileHelper::LoadFileToString(jsonStringTMX,
                                    ToCStr(FPaths::Combine(FPaths::ProjectDir(), TEXT("mapassignments.tmj")))))
  {
    TSharedPtr<FJsonObject> jsonTMX;
    TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<>::Create(jsonStringTMX);
    if (FJsonSerializer::Deserialize(JsonReader, jsonTMX))
    {
      FTiledMap TiledMap;
      FJsonObjectConverter::JsonObjectToUStruct<FTiledMap>(jsonTMX.ToSharedRef(), &TiledMap);

      for (int i = 0; i < TiledMap.layers[0].height; ++i)
        for (int j = 0; j < TiledMap.layers[0].width; ++j)
        {
          int dataIndex = i * TiledMap.layers[0].height + j;
          auto value = TiledMap.layers[0].data[dataIndex];
          if (value > 0 && loadedUnitTypes.Contains(value))
          {
            auto zaRow = loadedUnitTypes[value];
            auto zaunit = zaRow.unitData;
            zaunit.Id = dataIndex;
            zaunit.Name = FString::FormatAsNumber(dataIndex);
            Game->map.loadedUnits.Add(zaunit.Id, zaunit);

            FIntPoint positionPlacement{i, j};
            Game->map.unitAssignment.Add(positionPlacement, dataIndex);

            UClass* UnrealActor = zaRow.UnrealActor.Get();
            auto unitActor = GetWorld()->SpawnActor<ADungeonUnitActor>(UnrealActor, FVector::ZeroVector,
                                                                       FRotator::ZeroRotator);
            Game->unitIdToActor.Add(zaunit.Id, unitActor);
            unitActor->SetActorLocation(TilePositionToWorldPoint(positionPlacement));
          }
        }
    }
  }

  FString Result;
  FString Str = FPaths::Combine(FPaths::ProjectDir(), TEXT("UnitIds.csv"));
  if (FFileHelper::LoadFileToString(Result, ToCStr(Str)))
  {
    UDataTable* DataTable = NewObject<UDataTable>();
    DataTable->RowStruct = FStructThing::StaticStruct();
    DataTable->CreateTableFromCSVString(Result);
    TArray<FStructThing*> wow;
    DataTable->GetAllRows<FStructThing>(TEXT("wew"), wow);
    FStructThing* StructThing = DataTable->FindRow<FStructThing>(FName(TEXT("1")), TEXT("wow"));
    FString TableAsCSV = DataTable->GetTableAsCSV();
    FFileHelper::SaveStringToFile(TableAsCSV, ToCStr(FPaths::Combine(FPaths::ProjectDir(), TEXT("UnitIdsOutput.csv"))));
  }

  // FSimpleTileGraph gridGraph = FSimpleTileGraph(this->Game->map);
  // FGraphAStar<FSimpleTileGraph> graph = FGraphAStar<FSimpleTileGraph>(gridGraph);
  // TArray<FIntPoint> outPath;
  // EGraphAStarResult result = graph.FindPath(FSimpleTileGraph::FNodeRef{ 0, 0 }, FSimpleTileGraph::FNodeRef{ 5, 3 }, gridGraph, outPath);
  // for (int i = 0, j = 1; j < outPath.Num(); ++i, ++j)
  // {
  // 	SubmitLinearAnimation(Game->unitIdToActor.FindChecked(1), outPath[i], outPath[j], 0.25);
  // }

  AActor* tileShowPrefab = GetWorld()->SpawnActor<AActor>(TileShowPrefab, FVector::ZeroVector, FRotator::ZeroRotator);
  tileVisualizationComponent = static_cast<UTileVisualizationComponent*>(
    tileShowPrefab->GetComponentByClass(UTileVisualizationComponent::StaticClass()));

  CombatActionEvent.AddDynamic(this, &ADungeonGameModeBase::ApplyAction);

  AMapCursorPawn* mapPawn = static_cast<AMapCursorPawn*>(GetWorld()->GetFirstPlayerController()->GetPawn());
  if (mapPawn != NULL)
  {
    MainWidget = CreateWidget<UDungeonMainWidget>(this->GetWorld()->GetFirstPlayerController(), MainWidgetClass);
    MainWidget->Wait->OnClicked.AddUniqueDynamic(this, &ADungeonGameModeBase::HandleWait);
    MainWidget->AddToViewport();
    auto InAttribute = FCoreStyle::GetDefaultFontStyle("Bold", 30);
    InAttribute.OutlineSettings.OutlineSize = 3.0;
    style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
    style.SetColorAndOpacity(FSlateColor(FLinearColor::White));

    auto styleSetter = [&](FString&& fieldName, const auto& methodPointer) -> decltype(SVerticalBox::Slot())
    {
      return SVerticalBox::Slot().AutoHeight()[
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        [
          SNew(STextBlock)
           .TextStyle(&style)
           .Font(InAttribute)
           .Text(FText::FromString(fieldName))
        ]
        + SHorizontalBox::Slot()
        [
          SNew(STextBlock)
           .TextStyle(&style)
           .Font(InAttribute)
           .Text_UObject(this, methodPointer)
        ]
      ];
    };

    MainWidget->UnitDisplay->SetContent(
      SNew(SVerticalBox)
      + styleSetter("Name", &ADungeonGameModeBase::GetLastSeenUnitUnderCursorName)
      + styleSetter("HitPoints", &ADungeonGameModeBase::GetLastSeenUnitUnderCursorHitPoints)
      + styleSetter("Movement", &ADungeonGameModeBase::GetLastSeenUnitUnderCursorMovement)
    );

    baseState = MakeShared<FSelectingGameState>(*this);
    baseState->Enter();
  }
}

void ADungeonGameModeBase::React()
{
  FDungeonLogicMap& DungeonLogicMap = Game->map;
  for (auto LoadedUnit : DungeonLogicMap.loadedUnits)
  {
    TWeakObjectPtr<ADungeonUnitActor> DungeonUnitActor = Game->unitIdToActor.FindChecked(LoadedUnit.Key);
    DungeonUnitActor->React(LoadedUnit.Value);
    DungeonUnitActor->SetActorLocation(
      FVector(*DungeonLogicMap.unitAssignment.FindKey(LoadedUnit.Key)) * TILE_POINT_SCALE);
  }
}

void ADungeonGameModeBase::ApplyAction(FCombatAction action)
{
  Game->map.loadedUnits.Add(action.updatedUnit.Id, action.updatedUnit);
  FDungeonLogicUnit DungeonLogicUnit = Game->map.loadedUnits.FindChecked(action.InitiatorId);
  DungeonLogicUnit.state = UnitState::ActionTaken;
  UpdateUnitActor(action.updatedUnit);
  UpdateUnitActor(DungeonLogicUnit);
}

void ADungeonGameModeBase::HandleWait()
{
  CommitAndGotoBaseState();
}

void ADungeonGameModeBase::RefocusMenu()
{
  if (MainWidget->Move->HasUserFocus(0))
    return;
  MainWidget->Move->SetFocus();
}

FAttackResults ADungeonGameModeBase::AttackVisualization(int attackingUnitId)
{
  FDungeonLogicMap DungeonLogicMap = Game->map;
  auto attackingUnitPosition = DungeonLogicMap.unitAssignment.FindKey(attackingUnitId);
  FDungeonLogicUnit attackingUnit = DungeonLogicMap.loadedUnits.FindChecked(attackingUnitId);
  FAttackResults results;

  results.AllAttackTiles = DungeonLogicMap.constrainedManhattanReachablePoints<TSet<FIntPoint>>(
    attackingUnit.attackRange, *attackingUnitPosition);
  for (FIntPoint Point : results.AllAttackTiles)
  {
    if (DungeonLogicMap.unitAssignment.Contains(Point))
    {
      int unitId = DungeonLogicMap.unitAssignment[Point];
      FDungeonLogicUnit unitAtTile = DungeonLogicMap.loadedUnits[unitId];
      //TODO: Logic for determining friend foe?
      if (unitAtTile.teamId != attackingUnit.teamId)
      {
        results.AttackableUnitTiles.Add(Point);
      }
    }
  }
  return results;
}

TOptional<FDungeonLogicUnit> ADungeonGameModeBase::FindUnit(FIntPoint pt)
{
  FDungeonLogicMap DungeonLogicMap = this->Game->map;
  if (DungeonLogicMap.unitAssignment.Contains(pt))
  {
    return DungeonLogicMap.loadedUnits[DungeonLogicMap.unitAssignment[pt]];
  }
  return TOptional<FDungeonLogicUnit>();
}

void ADungeonGameModeBase::Tick(float deltaTime)
{
  Super::Tick(deltaTime);

  if (this->AnimationQueue.IsEmpty())
    return;

  (*this->AnimationQueue.Peek())->TickTimeline(deltaTime);
}

void ADungeonGameModeBase::UpdateUnitActor(const FDungeonLogicUnit& unit)
{
  FDungeonLogicMap& DungeonLogicMap = Game->map;
  TWeakObjectPtr<ADungeonUnitActor> DungeonUnitActor = Game->unitIdToActor.FindChecked(unit.Id);
  DungeonUnitActor->React(unit);
  DungeonUnitActor->SetActorLocation(
    FVector(*DungeonLogicMap.unitAssignment.FindKey(unit.Id)) * TILE_POINT_SCALE);
}

void FSelectingGameState::OnCursorPositionUpdate(FIntPoint CurrentPosition)
{
  FDungeonLogicMap& DungeonLogicMap = gameMode.Game->map;
  TMap<FIntPoint, int>& unitAssignmentMap = DungeonLogicMap.unitAssignment;
  TSet<FIntPoint> points;
  FAttackResults results;
  auto foundId = unitAssignmentMap.Contains(CurrentPosition) ? unitAssignmentMap.FindChecked(CurrentPosition) : -1;
  foundUnit = DungeonLogicMap.loadedUnits.Find(foundId);
  bool unitCanTakeAction = foundUnit != nullptr && !gameMode.Game->turnState.unitsFinished.Contains(foundId);
  if (unitCanTakeAction)
  {
    points = DungeonLogicMap.constrainedManhattanReachablePoints<TSet<FIntPoint>>(
      foundUnit->Movement + foundUnit->attackRange, CurrentPosition);
    points.Remove(CurrentPosition);
  }

  auto filterer = [&](FInt16Range range)
  {
    return [&](FIntPoint point)
    {
      auto distanceVector = point - CurrentPosition;
      auto distance = abs(distanceVector.X) + abs(distanceVector.Y);
      return range.Contains(distance);
    };
  };

  gameMode.tileVisualizationComponent->Clear();
  this->targets.Find(ETargetsAvailableId::move)->Empty();
  this->targets.Find(ETargetsAvailableId::attack)->Empty();

  if (foundUnit)
  {
    gameMode.LastSeenUnitUnderCursor = foundUnit;

    FInt16Range attackRangeFilter = FInt16Range(
      FInt16Range::BoundsType::Exclusive(foundUnit->Movement),
      FInt16Range::BoundsType::Inclusive(foundUnit->Movement + foundUnit->attackRange));
    TSet<FIntPoint> attackTiles;
    Algo::CopyIf(points, attackTiles, filterer(attackRangeFilter));
    FInt16Range movementRangeFilter = FInt16Range::Inclusive(0, foundUnit->Movement);
    TSet<FIntPoint> movementTiles;
    Algo::CopyIf(points, movementTiles, filterer(movementRangeFilter));

    gameMode.tileVisualizationComponent->ShowTiles(movementTiles, FLinearColor::Blue);
    gameMode.tileVisualizationComponent->ShowTiles(attackTiles, FLinearColor::Red);

    this->targets.Add(ETargetsAvailableId::move, movementTiles);
    this->targets.Add(ETargetsAvailableId::attack, movementTiles.Union(attackTiles));
  }
}

void FSelectingGameState::OnCursorQuery(FIntPoint pt)
{
  if (foundUnit != nullptr && !gameMode.Game->turnState.unitsFinished.Contains(foundUnit->Id))
  {
    gameMode.InputStateTransition(new FSelectingMenu(gameMode, pt, foundUnit, MoveTemp(this->targets)));
  }
}

void FSelectingGameState::Enter()
{
  AMapCursorPawn* MapCursorPawn = gameMode.GetMapCursorPawn();
  OnCursorPositionUpdate(MapCursorPawn->CurrentPosition);
  gameMode.MainWidget->MainMenu->SetVisibility(ESlateVisibility::Collapsed);

  //TODO: Hmm making the assumption that we're the set state? HMM
  auto queryDelegate = MapCursorPawn->QueryInput.AddRaw(this, &FSelectingGameState::OnCursorQuery);
  auto positionDelegate = MapCursorPawn->CursorEvent.AddRaw(this, &FSelectingGameState::OnCursorPositionUpdate);
  unregisterDelegates = [queryDelegate, positionDelegate, MapCursorPawn]()
  {
    MapCursorPawn->QueryInput.Remove(queryDelegate);
    MapCursorPawn->CursorEvent.Remove(positionDelegate);
  };
}

void FSelectingGameState::Exit()
{
  unregisterDelegates();
}

FReply FSelectingMenu::OnAttackButtonClick()
{
  TFunction<void(FIntPoint)> cursorQueryHandler =
    [ &, TilesExtent = targets.FindAndRemoveChecked(ETargetsAvailableId::attack) ]
  (FIntPoint pt)
  {
    TOptional<FDungeonLogicUnit> foundUnit = gameMode.FindUnit(pt);
    if (!TilesExtent.Contains(pt) || !foundUnit.IsSet() || foundUnit->teamId == initiatingUnit->teamId)
    {
      return false;
    }

    auto singleSubmitHandler =
      CastChecked<USingleSubmitHandler>(
        gameMode.AddComponentByClass(USingleSubmitHandler::StaticClass(), false, FTransform(), true));
    singleSubmitHandler->focusWorldLocation = TilePositionToWorldPoint(pt);
    singleSubmitHandler->totalLength = 3.;
    singleSubmitHandler->pivot = 0.5;
    singleSubmitHandler->fallOffsFromPivot = {0.1f, .2f, .3f};
    singleSubmitHandler->InteractionFinished.AddLambda(
      [this, foundUnit, pt]
    (const FInteractionResults& results)
      {
        int sourceID = initiatingUnit->Id;
        int targetID = foundUnit->Id;
        auto initiator = gameMode.Game->map.loadedUnits.FindChecked(sourceID);
        auto target = gameMode.Game->map.loadedUnits.FindChecked(targetID);
        auto damage = initiator.damage - floor((0.05 * results[0].order));
        target.HitPoints -= damage;

        gameMode.Reduce(TAction(TInPlaceType<FCombatAction>{}, sourceID, target, damage, pt));
      });
    gameMode.FinishAddComponent(singleSubmitHandler, false, FTransform());
    gameMode.InputStateTransition(new FAttackState(gameMode, singleSubmitHandler));

    return true;
  };

  gameMode.InputStateTransition(new FSelectingTargetGameState(gameMode, MoveTemp(cursorQueryHandler)));

  return FReply::Handled();
}

void ADungeonGameModeBase::Reduce(TAction unionAction)
{
  Visit(lager::visitor{[&](auto action)
  {
    int g = 0;
    using T = decltype(action);
    if constexpr (
      TOr<
      TIsSame<T, FCombatAction>,
      TIsSame<T, FMoveAction>
      >::Value)
    {
      FDungeonLogicUnit foundUnit = Game->map.loadedUnits.FindChecked(action.InitiatorId);
      foundUnit.state = UnitState::ActionTaken;
      Game->turnState.unitsFinished.Add(foundUnit.Id);

      Visit(lager::visitor{
              [&](FMoveAction action)
              {
                FDungeonLogicMap& DungeonLogicMap = Game->map;
                DungeonLogicMap.unitAssignment.Remove(*DungeonLogicMap.unitAssignment.FindKey(action.InitiatorId));
                DungeonLogicMap.unitAssignment.Add(action.Destination, action.InitiatorId);

                // FDungeonLogicUnit foundUnit = Game->map.loadedUnits.FindChecked(action.InitiatorId);
                // foundUnit.state = UnitState::ActionTaken;
                // Game->turnState.unitsFinished.Add(foundUnit.Id);

                UpdateUnitActor(foundUnit);
                CommitAndGotoBaseState();
              },
              [&](FCombatAction action)
              {
                CombatActionEvent.Broadcast(action);

                // FDungeonLogicUnit DungeonLogicUnit = Game->map.loadedUnits.FindChecked(action.InitiatorId);
                // DungeonLogicUnit.state = UnitState::ActionTaken;
                // Game->turnState.unitsFinished.Add(DungeonLogicUnit.Id);

                UpdateUnitActor(action.updatedUnit);
                UpdateUnitActor(foundUnit);
                CommitAndGotoBaseState();
              },
              [&](FEmptyVariantState action)
              {
              }
            }, unionAction);
    }
  }}, unionAction);

  // Visit(lager::visitor{
  //         [&](FMoveAction action)
  //         {
  //           FDungeonLogicMap& DungeonLogicMap = Game->map;
  //           DungeonLogicMap.unitAssignment.Remove(*DungeonLogicMap.unitAssignment.FindKey(action.InitiatorId));
  //           DungeonLogicMap.unitAssignment.Add(action.Destination, action.InitiatorId);
  //           
  //           FDungeonLogicUnit foundUnit = Game->map.loadedUnits.FindChecked(action.InitiatorId);
  //           foundUnit.state = UnitState::ActionTaken;
  //           Game->turnState.unitsFinished.Add(foundUnit.Id);
  //           
  //           UpdateUnitActor(foundUnit);
  //           CommitAndGotoBaseState();
  //         },
  //         [&](FCombatAction action)
  //         {
  //           CombatActionEvent.Broadcast(action);
  //           
  //           FDungeonLogicUnit DungeonLogicUnit = Game->map.loadedUnits.FindChecked(action.InitiatorId);
  //           DungeonLogicUnit.state = UnitState::ActionTaken;
  //           Game->turnState.unitsFinished.Add(DungeonLogicUnit.Id);
  //           
  //           UpdateUnitActor(action.updatedUnit);
  //           UpdateUnitActor(DungeonLogicUnit);
  //           CommitAndGotoBaseState();
  //         },
  //         [&](FEmptyVariantState action)
  //         {
  //         }
  //       }, unionAction);
}

FReply FSelectingMenu::OnMoveSelected()
{
  auto handleQueryTop = [
      this,
      TilesExtent = targets.FindAndRemoveChecked(ETargetsAvailableId::move)
    ](FIntPoint target)
  {
    if (gameMode.canUnitMoveToPointInRange(initiatingUnit->Id, target, TilesExtent))
    {
      gameMode.Reduce(TAction(TInPlaceType<FMoveAction>{}, initiatingUnit->Id, target));
    }
  };

  // auto Lambda = [handleQuery = static_cast<TFunction<void(FIntPoint)>>(handleQueryTop), this]()
  // {
  //   FDelegateHandle handle = gameMode.GetMapCursorPawn()->QueryInput.AddLambda(handleQuery);
  //   return [ handle, this]()
  //   {
  //     gameMode.GetMapCursorPawn()->QueryInput.Remove(handle);
  //   };
  // };

  // auto FStateWew = new FStateHook([this,innerHandle = handleQueryTop]()
  // {
  //   auto DelegateHandle = gameMode.GetMapCursorPawn()->QueryInput.AddLambda(innerHandle);
  //   return[&]()
  //   {
  //     gameMode.GetMapCursorPawn()->QueryInput.Remove(DelegateHandle);
  //   };
  // });

  gameMode.InputStateTransition<>(new FSelectingTargetGameState(gameMode, MoveTemp(handleQueryTop)));

  return FReply::Handled();
}
