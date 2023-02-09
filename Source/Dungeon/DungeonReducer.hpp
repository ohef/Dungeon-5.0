#pragma once
#include "Dungeon.h"
#include "Algo/Copy.h"
#include "Logic/DungeonGameState.h"
#include "lager/store.hpp"
#include "lager/util.hpp"
#include "lager/lenses/tuple.hpp"
#include "Lenses/model.hpp"
#include "Logic/util.h"
#include "Utility/VariantVisitor.hpp"

inline auto GetInteractionFields(FDungeonWorldState Model,
                                 int unitId) -> TTuple<TSet<FIntPoint>, TSet<FIntPoint>>
{
	auto foundUnitLogic = *lager::view(unitDataLens(unitId), Model);
	auto unitsPosition = lager::view(unitIdToPosition(unitId), Model);

	TSet<FIntPoint> points;

	bool unitCanTakeAction = !lager::view(isUnitFinishedLens2(unitId), Model).IsSet();
	int interactionDistance = foundUnitLogic.Movement + foundUnitLogic.attackRange;

	if (unitCanTakeAction)
	{
		points = manhattanReachablePoints(
			Model.Map.Width,
			Model.Map.Height,
			interactionDistance,
			unitsPosition);
		points.Remove(unitsPosition);
	}

	auto filterer = [&](FInt16Range range)
	{
		return [&](FIntPoint point)
		{
			auto distanceVector = point - unitsPosition;
			auto distance = abs(distanceVector.X) + abs(distanceVector.Y);
			return range.Contains(distance);
		};
	};

	FInt16Range attackRangeFilter = FInt16Range(
		FInt16Range::BoundsType::Exclusive(foundUnitLogic.Movement),
		FInt16Range::BoundsType::Inclusive(interactionDistance));
	TSet<FIntPoint> attackTiles;
	Algo::CopyIf(points, attackTiles, filterer(attackRangeFilter));
	FInt16Range movementRangeFilter = FInt16Range::Inclusive(0, foundUnitLogic.Movement);
	TSet<FIntPoint> movementTiles;
	Algo::CopyIf(points, movementTiles, filterer(movementRangeFilter));

	return {movementTiles, attackTiles};
}

#define TRANSITION_EDGE_FROM_TO(TFrom, TTo) \
  TTo operator()(TFrom& from) \
  {\
    return TTo();\
  }


struct FBackTransitions
{
	TRANSITION_EDGE_FROM_TO(FSelectingUnitContext, FMainMenu)
	TRANSITION_EDGE_FROM_TO(FMainMenu, FSelectingUnitContext)
	TRANSITION_EDGE_FROM_TO(FUnitMenu, FSelectingUnitContext)
	TRANSITION_EDGE_FROM_TO(FSelectingUnitAbilityTarget, FUnitMenu)
	TRANSITION_EDGE_FROM_TO(FUnitInteraction, FSelectingUnitAbilityTarget)
};

using FDungeonEffect = lager::effect<TDungeonAction>;
using FDungeonReducerResult = std::pair<FDungeonWorldState, FDungeonEffect>;

template <typename T>
inline void UpdateInteractionContext(FDungeonWorldState& worldState, T& Value)
{
	worldState.InteractionContext.Set<T>(MoveTemp(Value));
}

inline void UpdateInteractionContext(FDungeonWorldState& Model, FUnitInteraction& Value)
{
	const auto DungeonView = [&Model](auto&& Lens) { return lager::view(DUNGEON_FOWARD(Lens), Model); };
	auto possibleTarget = DungeonView(getUnitAtPointLens(DungeonView(cursorPositionLens)));
	Value.targetIDUnderFocus = *possibleTarget;

	Model.InteractionContext.Set<FUnitInteraction>(Value);
}

inline void UpdateInteractionContext(FDungeonWorldState& worldState, TInteractionContext& Value)
{
	Visit([&worldState](auto& x)
	{
		UpdateInteractionContext(worldState, x);
	}, Value);
}

FDungeonReducerResult AddInChangeStateEffect(auto& Model, auto&& ...values)
{
	return {
		Model, [=](const auto& ctx)
		{
			ctx.dispatch(TDungeonAction(TInPlaceType<FChangeState>{}, values...));
		}
	};
}

struct FCursorPositionUpdatedHandler : public TVariantVisitor<void, TInteractionContext>
{
	using TVariantVisitor<void, TInteractionContext>::operator();

	FDungeonWorldState& Model;
	FIntPoint& cursorPosition;

	FCursorPositionUpdatedHandler(FDungeonWorldState& Model, FIntPoint& CursorPosition)
		: Model(Model),
		  cursorPosition(CursorPosition)
	{
	}

	void operator()(FSelectingUnitContext& currentContext)
	{
		currentContext.interactionTiles.Find(ETargetsAvailableId::move)->Empty();
		currentContext.interactionTiles.Find(ETargetsAvailableId::attack)->Empty();

		currentContext.unitUnderCursor = lager::view(getUnitAtPointLens(cursorPosition), Model);
		if (!currentContext.unitUnderCursor.IsSet())
			return;

		auto [movementTiles, attackTiles] = GetInteractionFields(Model, *currentContext.unitUnderCursor);

		currentContext.interactionTiles.Add(ETargetsAvailableId::move, movementTiles);
		currentContext.interactionTiles.Add(ETargetsAvailableId::attack, movementTiles.Union(attackTiles));
	}
};

inline auto WorldStateReducer(FDungeonWorldState Model, TDungeonAction worldAction) -> FDungeonReducerResult
{
	const auto DungeonView = [&Model](auto&& Lens) { return lager::view(DUNGEON_FOWARD(Lens), Model); };
	const auto DefaultPassthrough = [&](auto& x) -> FDungeonReducerResult { return {Model, lager::noop}; };
	const auto Matcher = Dungeon::Match(DefaultPassthrough);

	return Matcher
	([&](FBackAction&) -> FDungeonReducerResult
	 {
		 return Visit(lager::visitor{
			              [&](auto& all) -> FDungeonReducerResult
			              {
				              auto Value = FBackTransitions{}(all);
				              Model.InteractionContext.Set<decltype(Value)>(Value);
				              return {Model, lager::noop};
			              },
		              }, Model.InteractionContext);
	 },
	 [&](FSpawnUnit& action) -> FDungeonReducerResult
	 {
		 const auto& Lens = lager::lenses::fan(
			 unitDataLens(action.unit.Id),
			 getUnitAtPointLens(action.position));
		 auto UpdatedModel = lager::set(Lens, Model, std::make_tuple(TOptional(action.unit), TOptional(action.unit.Id)));
		 return {UpdatedModel, lager::noop};
	 },
	 [&](FTimingInteractionResults& action) -> FDungeonReducerResult
	 {
		 return Matcher
		 ([&](FCombatAction& waitingAction) -> FDungeonReducerResult
		 {
			 auto damage = DungeonView(unitDataLens(waitingAction.InitiatorId))->damage
				 - floor((0.05 * action[0].order));

			 waitingAction.damageValue = damage;

			 Model.InteractionsToResolve.Pop();

			 return {
				 Model, [waitingAction = MoveTemp(waitingAction)](auto ctx)
				 {
					 ctx.dispatch(
						 TDungeonAction(TInPlaceType<FCombatAction>{}, waitingAction));
				 }
			 };
		 })(Model.WaitingForResolution);
	 },
	 [&](FCursorQueryTarget& actionTarget) -> FDungeonReducerResult
	 {
		 return Matcher
		 ([&](FSelectingUnitAbilityTarget&) -> FDungeonReducerResult
		  {
			  return Matcher
			  ([&](FMoveAction waitingAction) -> FDungeonReducerResult
			   {
				   bool bIsUnitThere = DungeonView(getUnitAtPointLens(actionTarget.target)).IsSet();
				   auto [movementTiles, attackTiles] = GetInteractionFields(Model, waitingAction.InitiatorId);
				   if (bIsUnitThere || !movementTiles.Contains(actionTarget.target))
					   return {Model, lager::noop};

				   waitingAction.Destination = actionTarget.target;
				   // Model.InteractionsToResolve.Pop();
				   return {
					   Model,
					   [waitingAction = MoveTemp(waitingAction)](auto ctx)
					   {
						   ctx.dispatch(
							   TDungeonAction(
								   TInPlaceType<FChangeState>{},
								   TInteractionContext(
									   TInPlaceType<FUnitMenu>{},
									   waitingAction.InitiatorId,
									   TSet<FName>({"MoveAction"})))
						   );
						   ctx.dispatch(TDungeonAction(
							   TInPlaceType<FMoveAction>{},
							   waitingAction));
					   }
				   };
			   },
			   [&](FCombatAction& waitingAction) -> FDungeonReducerResult
			   {
				   TOptional<FDungeonLogicUnit> initiatingUnit =
					   DungeonView(
						   unitDataLens(waitingAction.InitiatorId));
				   int targetedUnitId = DungeonView(
					   getUnitAtPointLens(actionTarget.target) |
					   unreal_value_or(-1));
				   TOptional<FDungeonLogicUnit> targetedUnit =
					   DungeonView(unitDataLens(targetedUnitId));

				   auto [movementTiles, attackTiles] =
					   GetInteractionFields(
						   Model, waitingAction.InitiatorId);

				   bool isInRange = attackTiles.Union(movementTiles).
				                                Contains(actionTarget.target);
				   bool isUnitThere = targetedUnit.IsSet();
				   bool isTargetNotOnTheSameTeam = targetedUnit.IsSet()
					   && targetedUnit->teamId != initiatingUnit->
					   teamId;
				   if (isInRange && isUnitThere &&
					   isTargetNotOnTheSameTeam)
				   {
					   waitingAction.targetedUnit = targetedUnit->Id;
				   	
					   Model.InteractionsToResolve.Pop();

					   auto val = Model.InteractionsToResolve.Top().
					                    TryGet<FUnitInteraction>();
					   val->originatorID = waitingAction.InitiatorId;
					   auto possibleTarget = DungeonView(
						   getUnitAtPointLens(
							   DungeonView(cursorPositionLens)));
					   val->targetIDUnderFocus = *possibleTarget;

					   return {
						   Model,
						   [interaction = Model.InteractionsToResolve.Top()](auto ctx)
						   {
							   ctx.dispatch(
								   TDungeonAction(
									   TInPlaceType<FChangeState>{},
									   interaction));
						   }
					   };
				   }

				   return DefaultPassthrough(waitingAction);
			   }
			  )(Model.WaitingForResolution);
		  },
		  [&](FSelectingUnitContext&) -> FDungeonReducerResult
		  {
			  auto foundUnitOpt = Dungeon::Selectors::GetUnitIdUnderCursor(Model);  
			  if (!foundUnitOpt.IsSet())
				  return {Model, lager::noop};

			  auto foundUnitId = *foundUnitOpt;
			  auto isFinished = !DungeonView(isUnitFinishedLens2(foundUnitId)).IsSet();

			  const auto& TurnState = DungeonView(attr(&FDungeonWorldState::TurnState));
			  auto DungeonLogicUnit = DungeonView(
				  unitDataLens(foundUnitId) | ignoreOptional);

			  auto isOnTeam = TurnState.teamId == DungeonLogicUnit.teamId;
			  if (isFinished && isOnTeam)
			  {
				  return {
					  Model, [foundUnitId](auto& ctx)
					  {
						  ctx.dispatch(
							  TDungeonAction(TInPlaceType<FChangeState>{},
							                 TInteractionContext(
								                 TInPlaceType<FUnitMenu>{},
								                 foundUnitId)));
					  }
				  };
			  }
			  else
			  {
				  return {Model, lager::noop};
			  }
		  }
		 )(Model.InteractionContext);
	 },
	 [&](FSteppedAction& action) -> FDungeonReducerResult
	 {
		 Model.InteractionsToResolve = action.interactions;
		 Model.WaitingForResolution = action.mainAction;
		 return AddInChangeStateEffect(Model, action.interactions.Top());
	 },
	 [&](FWaitAction& action) -> FDungeonReducerResult
	 {
		 Model.TurnState.unitsFinished.Add(action.InitiatorId);
		 Model.InteractionContext.Set<FSelectingUnitContext>({});

		 return {
			 Model, [](auto& ctx)
			 {
				 ctx.dispatch(TDungeonAction(TInPlaceType<FCommitAction>{}));
			 }
		 };
	 },
	 [&](FChangeState& action) -> FDungeonReducerResult
	 {
		 UpdateInteractionContext(Model, action.newState);
		 return {Model, lager::noop};
	 },
	 [&](FEndTurnAction& action) -> FDungeonReducerResult
	 {
		 const int MAX_PLAYERS = 2;
		 const auto nextTeamId = (Model.TurnState.teamId % MAX_PLAYERS) + 1;

		 Model.TurnState = {nextTeamId};
		 Model.InteractionContext.Set<FSelectingUnitContext>(FSelectingUnitContext());
		 return {Model, lager::noop};
	 },
	 [&](FCursorPositionUpdated& action) -> FDungeonReducerResult
	 {
		 Visit(FCursorPositionUpdatedHandler(Model, action.cursorPosition), Model.InteractionContext);
		 Model.CursorPosition = action.cursorPosition;
		 return {Model, lager::noop};
	 },
	 [&](FMoveAction& MoveAction) -> FDungeonReducerResult
	 {
		 FDungeonLogicMap& DungeonLogicMap = Model.Map;
		 DungeonLogicMap.UnitAssignment.Remove(
			 *DungeonLogicMap.UnitAssignment.FindKey(MoveAction.InitiatorId));
		 DungeonLogicMap.UnitAssignment.
		                 Add(MoveAction.Destination, MoveAction.InitiatorId);

		 return DefaultPassthrough(MoveAction);
	 },
	 [&](FCombatAction& action) -> FDungeonReducerResult
	 {
		 FDungeonLogicUnit& foundUnit = Model.Map.LoadedUnits.FindChecked(action.InitiatorId);
		 Model.TurnState.unitsFinished.Add(foundUnit.Id);

		 Model = lager::over(unitIdToData(action.targetedUnit), Model, [&](FDungeonLogicUnit Unit)
		 {
			 Unit.state = UnitState::ActionTaken;
			 Unit.HitPoints -= action.damageValue;
			 return Unit;
		 });
	 	
		 Model.InteractionContext.Set<FSelectingUnitContext>({});
	 	
		 return {
			 Model, [](auto& ctx)
			 {
				 ctx.dispatch(TDungeonAction(TInPlaceType<FCommitAction>{}));
			 }
		 };
	 },
	 [&](FFocusChanged& action) -> FDungeonReducerResult
	 {
		 Model.InteractionContext.Get<FUnitMenu>().focusedAbilityName = action.focusName;
		 return DefaultPassthrough(action);
	 }
	)(worldAction);
}

template <typename ...TArgs>
bool IsInTypeSet(const auto& variant)
{
	return (variant.template IsType<TArgs>() || ...);
}

const auto WithUndoReducer = [](auto&& reducer)
{
	return [reducer](auto next)
	{
		return [next,reducer](auto action,
		                      auto&& model,
		                      auto&&,
		                      auto&& loop,
		                      auto&& deps,
		                      auto&& tags)
		{
			return next(action,
			            LAGER_FWD(model),
			            [reducer](auto historyModel, auto a) -> std::pair<FHistoryModel, FDungeonEffect>
			            {
				            auto&& [childModel, eff] = reducer(historyModel.CurrentState(), a);

				            //TODO: Same kinda issue hard coding against the undo action is eh
				            if (IsInTypeSet<FBackAction>(a) && historyModel.CanGoBack())
				            {
					            return {historyModel.GoBack(), lager::noop};
				            }

				            //TODO: this exists because we want to activate undo on certain actions...probably should be in
				            //it's own store
				            if (IsInTypeSet<FChangeState>(a))
				            {
					            historyModel.AddRecord(historyModel.CurrentState());
					            historyModel.CurrentState() = childModel;
					            return {historyModel, eff};
				            }

				            historyModel.CurrentState() = childModel;

				            //TODO: does a rollback history need to also prevent undoing? 
				            if (IsInTypeSet<FCommitAction>(a))
				            {
					            historyModel.Commit();
				            }
				            return {historyModel, eff};
			            },
			            LAGER_FWD(loop),
			            LAGER_FWD(deps),
			            LAGER_FWD(tags));
		};
	};
};
