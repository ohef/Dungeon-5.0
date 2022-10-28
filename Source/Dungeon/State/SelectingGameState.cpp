#include "SelectingGameState.h"

#include "SelectingMenu.h"
#include "Algo/Copy.h"
#include "Dungeon/Lenses/model.hpp"
#include "lager/lenses.hpp"
#include "lager/state.hpp"
#include "Logic/util.h"

FSelectingGameState::FSelectingGameState(ADungeonGameModeBase& GameMode): gameMode(GameMode), foundUnit(nullptr)
{
  targets.Add(ETargetsAvailableId::move, TSet<FIntPoint>());
  targets.Add(ETargetsAvailableId::attack, TSet<FIntPoint>());
}

void FSelectingGameState::OnCursorPositionUpdate(FIntPoint CurrentPosition)
{
  gameMode.TileVisualizationComponent->Clear();
  this->targets.Find(ETargetsAvailableId::move)->Empty();
  this->targets.Find(ETargetsAvailableId::attack)->Empty();

  auto& DungeonLogicMap = lager::view(mapLens, gameMode);
  auto& unitAssignmentMap = DungeonLogicMap.unitAssignment;
  TSet<FIntPoint> points;
  bool hasUnit = unitAssignmentMap.Contains(CurrentPosition);

  if (!hasUnit)
  {
    foundUnit = nullptr;
    return;
  }

  auto foundId = unitAssignmentMap.FindChecked(CurrentPosition);
  foundUnit = DungeonLogicMap.loadedUnits.Find(foundId);

  bool unitCanTakeAction = !lager::view(isUnitFinishedLens(foundUnit->Id), gameMode).has_value();
  int interactionDistance = foundUnit->Movement + foundUnit->attackRange;

  if (unitCanTakeAction)
  {
    points = manhattanReachablePoints(
      DungeonLogicMap.Width,
      DungeonLogicMap.Height,
      interactionDistance,
      CurrentPosition);
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

  gameMode.LastSeenUnitUnderCursor = foundUnit;

  FInt16Range attackRangeFilter = FInt16Range(
    FInt16Range::BoundsType::Exclusive(foundUnit->Movement),
    FInt16Range::BoundsType::Inclusive(interactionDistance));
  TSet<FIntPoint> attackTiles;
  Algo::CopyIf(points, attackTiles, filterer(attackRangeFilter));
  FInt16Range movementRangeFilter = FInt16Range::Inclusive(0, foundUnit->Movement);
  TSet<FIntPoint> movementTiles;
  Algo::CopyIf(points, movementTiles, filterer(movementRangeFilter));

  gameMode.TileVisualizationComponent->ShowTiles(movementTiles, FLinearColor::Blue);
  gameMode.TileVisualizationComponent->ShowTiles(attackTiles, FLinearColor::Red);

  this->targets.Add(ETargetsAvailableId::move, movementTiles);
  this->targets.Add(ETargetsAvailableId::attack, movementTiles.Union(attackTiles));
}

void FSelectingGameState::OnCursorQuery(FIntPoint pt)
{
  if (foundUnit == nullptr)
    return;

  if (!lager::view(isUnitFinishedLens(foundUnit->Id), gameMode).has_value()
    && lager::view(turnStateLens, gameMode).teamId == foundUnit->teamId)
  {
    gameMode.InputStateTransition(new FSelectingMenu(gameMode, pt, foundUnit, MoveTemp(this->targets)));
  }
}

void FSelectingGameState::Enter()
{
  AMapCursorPawn* MapCursorPawn = gameMode.GetMapCursorPawn();
  OnCursorPositionUpdate(MapCursorPawn->CurrentPosition);
  gameMode.MainWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Collapsed);

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

