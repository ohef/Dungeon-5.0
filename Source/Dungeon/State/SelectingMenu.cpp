#include "SelectingMenu.h"

#include "AttackState.h"
#include "DungeonConstants.h"
#include "GraphAStar.h"
#include "SelectingTargetGameState.h"
#include "Algo/Accumulate.h"
#include "Logic/SimpleTileGraph.h"
#include "Logic/util.h"

void FSelectingMenu::Enter()
{
  AMapCursorPawn* MapCursorPawn = gameMode.GetMapCursorPawn();

  if (MapCursorPawn->CurrentPosition != capturedCursorPosition)
  {
    MapCursorPawn->SetActorLocation(TilePositionToWorldPoint(capturedCursorPosition));
  }

  MapCursorPawn->MovementComponent->ToggleActive();
  gameMode.MainWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Visible);
  // gameMode.RefocusMenu();

  //TODO: eh could template this somehow
  StaticCastSharedRef<SButton>(gameMode.MainWidget->Move->TakeWidget())
    ->SetOnClicked(FOnClicked::CreateSP(this->AsShared(), &FSelectingMenu::OnMoveSelected));

  StaticCastSharedRef<SButton>(gameMode.MainWidget->Attack->TakeWidget())
    ->SetOnClicked(FOnClicked::CreateSP(this->AsShared(), &FSelectingMenu::OnAttackButtonClick));

  StaticCastSharedRef<SButton>(gameMode.MainWidget->Wait->TakeWidget())
    ->SetOnClicked(FOnClicked::CreateSP(this->AsShared(), &FSelectingMenu::OnWaitButtonClick));
}

void FSelectingMenu::Exit()
{
  AMapCursorPawn* MapCursorPawn = gameMode.GetMapCursorPawn();
  MapCursorPawn->MovementComponent->ToggleActive();
  gameMode.MainWidget->UnitActionMenu->SetVisibility(ESlateVisibility::Collapsed);
}

FReply FSelectingMenu::OnAttackButtonClick()
{
  TFunction<void(FIntPoint)> cursorQueryHandler =
    [ &, TilesExtent = targets.FindChecked(ETargetsAvailableId::attack) ]
  (FIntPoint pt)
  {
    UKismetSystemLibrary::PrintString(&gameMode);
    // TOptional<FDungeonLogicUnit> foundUnit = gameMode.FindUnit(pt);
    TOptional<FDungeonLogicUnit> foundUnit;
    if (!TilesExtent.Contains(pt) || !foundUnit.IsSet() || foundUnit->teamId == initiatingUnit->teamId)
    {
      return false;
    }

    //move unit, if applicable...
    auto result = [&](int range, int maxPathLength, TArray<FIntPoint> suggestedMovement, FIntPoint currentPosition,
                      FIntPoint target)
    {
      auto suggestedAttackingPoint = suggestedMovement.Last();
      auto attackableTiles = manhattanReachablePoints(999, 999, range, target);
      attackableTiles.Remove(target);
      if (attackableTiles.Contains(suggestedAttackingPoint))
        return suggestedMovement;

      int solutionLength = 999999;
      auto output = Algo::Accumulate(attackableTiles, TArray<FIntPoint>(), [&](auto acc, auto val)
      {
        TArray<FIntPoint> daResult;
        FSimpleTileGraph SimpleTileGraph = FSimpleTileGraph(gameMode.Game.map, maxPathLength);
        FGraphAStar aStarGraph(SimpleTileGraph);
        aStarGraph.FindPath(currentPosition, target, SimpleTileGraph, daResult);

        if (solutionLength > daResult.Num())
        {
          solutionLength = daResult.Num();
          return daResult;
        }
        else
        {
          return acc;
        }
      });

      return output;
    }(initiatingUnit->attackRange, initiatingUnit->Movement, {{-1, -1}}, capturedCursorPosition, pt);

    auto singleSubmitHandler =
      CastChecked<USingleSubmitHandler>(
        gameMode.AddComponentByClass(USingleSubmitHandler::StaticClass(), false, FTransform(), true));
    singleSubmitHandler->focusWorldLocation = TilePositionToWorldPoint(pt);
    singleSubmitHandler->totalLength = 3.;
    singleSubmitHandler->pivot = 1.5;
    singleSubmitHandler->fallOffsFromPivot = {0.3f};
    singleSubmitHandler->InteractionFinished.AddLambda(
      [this, foundUnit, pt]
    (const FInteractionResults& results)
      {
        int sourceID = initiatingUnit->Id;
        int targetID = foundUnit->Id;
        auto initiator = gameMode.Game.map.loadedUnits.FindChecked(sourceID);
        auto target = gameMode.Game.map.loadedUnits.FindChecked(targetID);
        auto damage = initiator.damage - floor((0.05 * results[0].order));
        target.HitPoints -= damage;

        gameMode.Dispatch(TDungeonAction(TInPlaceType<FCombatAction>{}, sourceID, target, damage, pt));
      });
      
    gameMode.FinishAddComponent(singleSubmitHandler, false, FTransform());
    gameMode.InputStateTransition(new FAttackState(gameMode, singleSubmitHandler));

    return true;
  };

  gameMode.InputStateTransition(new FSelectingTargetGameState(gameMode, MoveTemp(cursorQueryHandler)));

  return FReply::Handled();
}

FReply FSelectingMenu::OnWaitButtonClick()
{
  gameMode.Dispatch(TDungeonAction(TInPlaceType<FWaitAction>{}, this->initiatingUnit->Id));
  return FReply::Handled();
}

FReply FSelectingMenu::OnMoveSelected()
{
  auto handleQueryTop = [
      this,
      TilesExtent = targets.FindAndRemoveChecked(ETargetsAvailableId::move)
    ](FIntPoint target)
  {
    // if (gameMode.canUnitMoveToPointInRange(initiatingUnit->Id, target, TilesExtent))
    if (false)
    {
      gameMode.Dispatch(TDungeonAction(TInPlaceType<FMoveAction>{}, initiatingUnit->Id, target));
    }
  };

  gameMode.InputStateTransition<>(new FSelectingTargetGameState(gameMode, MoveTemp(handleQueryTop)));

  return FReply::Handled();
}
