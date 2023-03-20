#pragma once
#include "Animation/AnimInstance.h"
#include "Dungeon.h"
#include "DungeonConstants.h"
#include "DungeonUnitActor.h"
#include "GraphAStar.h"
#include "PromiseFulfiller.h"
#include "Algo/Copy.h"
#include "Algo/ForEach.h"
#include "Logic/DungeonGameState.h"
#include "lager/store.hpp"
#include "lager/util.hpp"
#include "lager/lenses/tuple.hpp"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"
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
using FDungeonReducerResult = lager::result<FDungeonWorldState, TDungeonAction>;

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

//UHHMMM GROSS DEPARTMENT?
template <typename TDelegate, bool TReturnTuple = false>
inline decltype(auto) CreateResolvingFutureOnEventRaise(TDelegate& OnInterpToStopDelegate, const auto& context,
	typename TDelegate::FDelegate::template TMethodPtrResolver< UPromiseFulfiller >::FMethodPtr FuncPtr,
	FName InFunctionName )
{
	auto PromiseFulfiller = NewObject<UPromiseFulfiller>();
	auto [p,f] = lager::promise::with_loop(context.loop());
	PromiseFulfiller->promiseToFulfill = MoveTemp(p);
	OnInterpToStopDelegate.__Internal_AddUniqueDynamic(PromiseFulfiller, FuncPtr, InFunctionName );
	auto chainedFuture = MoveTemp(f).then(
		[&OnInterpToStopDelegate, PromiseFulfiller, FuncPtr, InFunctionName ]
		{
			OnInterpToStopDelegate.__Internal_RemoveDynamic(PromiseFulfiller, FuncPtr, InFunctionName);
		});
	
	if constexpr (TReturnTuple)
		return MakeTuple(PromiseFulfiller, MoveTemp(chainedFuture));
	else
		return chainedFuture;
};

#define CreateFutureFromUnrealDelegate(Delegate, Context, FuncName) CreateResolvingFutureOnEventRaise( Delegate, Context, FuncName, STATIC_FUNCTION_FNAME( TEXT( #FuncName ) ) )
#define CreatePromiseFromUnrealDelegate(Delegate, Context, FuncName) CreateResolvingFutureOnEventRaise<decltype(Delegate), true>( Delegate, Context, FuncName, STATIC_FUNCTION_FNAME( TEXT( #FuncName ) ) )

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

const auto IsNull = [](auto... ptr)
{
	return ( (ptr == nullptr) && ... );
};

const auto identity = []{};

inline auto WorldStateReducer(FDungeonWorldState Model, TDungeonAction worldAction, TFunctionRef<void()>&& signalCheckpoint = identity) -> FDungeonReducerResult
{
	const auto ReducerView = [&Model](auto&& Lens) { return lager::view(DUNGEON_FOWARD(Lens), Model); };
	const auto DefaultPassthrough = [&](auto& x) -> FDungeonReducerResult { return {Model, lager::noop}; };
	const auto Matcher = Dungeon::Match(DefaultPassthrough);

	return Matcher
	([&](FBackAction&) -> FDungeonReducerResult
	 {
		 return Dungeon::MatchAgnostic(
			 [&](auto& all) -> FDungeonReducerResult
			 {
				 auto Value = FBackTransitions{}(all);
				 Model.InteractionContext.Set<decltype(Value)>(Value);
				 return Model;
			 }
		 )(Model.InteractionContext);
		
		return Model;
	 },
	 [&](FSpawnUnit& action) -> FDungeonReducerResult
	 {
		 const auto& Lens = lager::lenses::fan(
			 unitDataLens(action.unit.Id),
			 getUnitAtPointLens(action.position));
		 auto UpdatedModel = lager::set(Lens, Model, std::make_tuple(TOptional(action.unit), TOptional(action.unit.Id)));
		 return UpdatedModel;
	 },
	 [&](FTimingInteractionResults& action) -> FDungeonReducerResult
	 {
		 return Matcher
		 ([&](FCombatAction& waitingAction) -> FDungeonReducerResult
		 {
			 auto damage = ReducerView(unitDataLens(waitingAction.InitiatorId))->damage
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
				   bool bIsUnitThere = ReducerView(getUnitAtPointLens(actionTarget.target)).IsSet();
				   auto [movementTiles, attackTiles] = GetInteractionFields(Model, waitingAction.InitiatorId);
				   if (bIsUnitThere || !movementTiles.Contains(actionTarget.target))
					   return {Model, lager::noop};

				   signalCheckpoint();
				   waitingAction.Destination = actionTarget.target;
			  	
				   return {
					   Model,
					   [DUNGEON_MOVE_LAMBDA(waitingAction = MoveTemp(waitingAction)), signalCheckpoint](auto ctx)
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
					   ReducerView(
						   unitDataLens(waitingAction.InitiatorId));
				   int targetedUnitId = ReducerView(
					   getUnitAtPointLens(actionTarget.target) |
					   unreal_value_or(-1));
				   TOptional<FDungeonLogicUnit> targetedUnit =
					   ReducerView(unitDataLens(targetedUnitId));

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
					   signalCheckpoint();
					   
					   waitingAction.targetedUnit = targetedUnit->Id;
				   	
					   Model.InteractionsToResolve.Pop();

					   auto val = Model.InteractionsToResolve.Top().
					                    TryGet<FUnitInteraction>();
					   val->originatorID = waitingAction.InitiatorId;
					   auto possibleTarget = ReducerView(
						   getUnitAtPointLens(
							   ReducerView(cursorPositionLens)));
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

				   return Model;
			   }
			  )(Model.WaitingForResolution);
		  },
		  [&](FSelectingUnitContext&) -> FDungeonReducerResult
		  {
			  auto foundUnitOpt = Dungeon::Selectors::GetUnitIdUnderCursor(Model);
			  if (!foundUnitOpt.IsSet())
				  return {Model, lager::noop};

			  auto foundUnitId = *foundUnitOpt;
			  auto isFinished = !ReducerView(isUnitFinishedLens2(foundUnitId)).IsSet();

			  const auto& TurnState = ReducerView(attr(&FDungeonWorldState::TurnState));
			  auto DungeonLogicUnit = ReducerView(
				  unitDataLens(foundUnitId) | ignoreOptional);

			  auto isOnTeam = TurnState.teamId == DungeonLogicUnit.teamId;
			  if (isFinished && isOnTeam)
			  {
				  signalCheckpoint();
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
				  return Model;
			  }
		  }
		 )(Model.InteractionContext);
	 },
	 [&](FSteppedAction& action) -> FDungeonReducerResult
	 {
		 Model.InteractionsToResolve = action.interactions;
		 Model.WaitingForResolution = action.mainAction;
		 signalCheckpoint();
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
		 return Model;
	 },
	 [&](FEndTurnAction& action) -> FDungeonReducerResult
	 {
		 const int MAX_PLAYERS = 2;
		 const auto nextTeamId = (Model.TurnState.teamId % MAX_PLAYERS) + 1;

		 Model.TurnState = {nextTeamId};
		 Model.InteractionContext.Set<FSelectingUnitContext>(FSelectingUnitContext());
		 return Model;
	 },
	 [&](FCursorPositionUpdated& action) -> FDungeonReducerResult
	 {
		 Visit(FCursorPositionUpdatedHandler(Model, action.cursorPosition), Model.InteractionContext);
		 Model.CursorPosition = action.cursorPosition;
		 return Model;
	 },
	 [&](FMoveAction& MoveAction) -> FDungeonReducerResult
	 {
		 FDungeonLogicMap DungeonLogicMap = Model.Map;
		 const FDungeonLogicUnit& DungeonLogicUnit = ReducerView(unitIdToData(MoveAction.InitiatorId));
		 FIntPoint LastPosition = *DungeonLogicMap.UnitAssignment.FindKey(MoveAction.InitiatorId);

		 int before = DungeonLogicMap.UnitAssignment.Num();

		 DungeonLogicMap.UnitAssignment.Remove(LastPosition);
		 DungeonLogicMap.UnitAssignment.Add(MoveAction.Destination, MoveAction.InitiatorId);

		 int after = DungeonLogicMap.UnitAssignment.Num();

		 if (before != after)
			 UE_DEBUG_BREAK();

		 Model.Map = DungeonLogicMap;

		 auto actor = ReducerView(unitIdToActor(MoveAction.InitiatorId));

		 return {
			 Model,
			 [actor, DungeonLogicMap, MoveAction, DungeonLogicUnit, LastPosition]
		 (TDungeonStore::context_t context)
			 {
				 if (!actor.IsSet()) //TODO Fuck this runs in unit tests
					 return lager::future{};

				 auto actorPtr = *actor;
				 auto moveComp = actorPtr->InterpToMovementComponent;
				 moveComp->ResetControlPoints();

				 FIntPoint UpdatedPosition = MoveAction.Destination;

				 TArray<FIntPoint> aStarResult;
				 FSimpleTileGraph SimpleTileGraph = FSimpleTileGraph(DungeonLogicMap, DungeonLogicUnit.Movement);
				 FGraphAStar aStarGraph(SimpleTileGraph);
				 auto Result = aStarGraph.FindPath(LastPosition, UpdatedPosition, SimpleTileGraph, aStarResult);

				 moveComp->ResetControlPoints();
				 moveComp->Duration = aStarResult.Num() / 3.0f;
				 moveComp->AddControlPointPosition(TilePositionToWorldPoint(LastPosition), false);
				 Algo::ForEach(aStarResult, [&](auto x) mutable
				 {
					 moveComp->AddControlPointPosition(TilePositionToWorldPoint(x), false);
				 });
				 moveComp->FinaliseControlPoints();
				 moveComp->RestartMovement();

				 //LMAO - Yeah you can do this
				 auto springArm = actorPtr
					->GetWorld()
					->GetFirstPlayerController()
					->GetPawn()
					->FindComponentByClass<USpringArmComponent>();

				 auto oldParent = springArm->GetAttachParent();
				 springArm->AttachToComponent(actorPtr->GetRootComponent(),
				                              FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));

				 return CreateFutureFromUnrealDelegate(moveComp->OnInterpToStop, context,
				                                       &UPromiseFulfiller::HandleOnInterpToStop)
					 .then([springArm, actorPtr, oldParent]
					 {
						 springArm->AttachToComponent(
							 oldParent, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
					 });
			 }
		 };
	 },
	 [&](FCombatAction& action) -> FDungeonReducerResult
	 {
		 FDungeonLogicUnit& initiatorUnit = Model.Map.LoadedUnits.FindChecked(action.InitiatorId);

		 auto actorMap = ReducerView(attr(&FDungeonWorldState::unitIdToActor));
		 auto initatorActor = actorMap.FindAndRemoveChecked(initiatorUnit.Id);
		 actorMap.Remove(action.targetedUnit);
	 	
		 using TComp = USkeletalMeshComponent;
		 TArray<TComp*> comps;
		 for (TTuple<int, TWeakObjectPtr<ADungeonUnitActor>> tuple : actorMap)
		 {
			 tuple.Value->GetComponents(comps);
			 for (auto c : comps)
			 {
				 c->SetVisibleFlag(false);
				 c->MarkRenderStateDirty();
			 }
		 }

		 Model.TurnState.unitsFinished.Add(initiatorUnit.Id);

		 Model = lager::over(unitIdToData(action.targetedUnit), Model, [&](FDungeonLogicUnit Unit)
		 {
			 Unit.state = UnitState::ActionTaken;
			 Unit.HitPoints -= action.damageValue;
			 return Unit;
		 });

		 Model.InteractionContext.Set<FSelectingUnitContext>({});

		 return {
			 Model, [initatorActor, DUNGEON_MOVE_LAMBDA(actorMap)](auto& ctx) -> lager::future
			 {
				 auto FindComponentByClass = initatorActor
					 ->FindComponentByClass<USkeletalMeshComponent>();
				 UAnimInstance* AnimInstance = FindComponentByClass
					 ->GetAnimInstance();

				 if (IsNull(AnimInstance, initatorActor->CombatActionMontage))
					 return {};

				 AnimInstance->Montage_Play(initatorActor->CombatActionMontage);

				 TTuple<UPromiseFulfiller*, lager::future> tuple = CreatePromiseFromUnrealDelegate(AnimInstance->OnMontageEnded, ctx, &UPromiseFulfiller::HandleOnMontageEnded);

				 return CreateFutureFromUnrealDelegate(AnimInstance->OnMontageEnded, ctx,
				                                       &UPromiseFulfiller::HandleOnMontageEnded)
					 .then([&ctx, DUNGEON_MOVE_LAMBDA(actorMap)]
					 {
						 using TComp = USkeletalMeshComponent;
						 TArray<TComp*> comps;
						 for (TTuple<int, TWeakObjectPtr<ADungeonUnitActor>> tuple : actorMap)
						 {
							 tuple.Value->GetComponents(comps);
							 for (auto c : comps)
							 {
								 c->SetVisibleFlag(true);
								 c->MarkRenderStateDirty();
							 }
						 }
					 	
						 ctx.dispatch(TDungeonAction(TInPlaceType<FCommitAction>{}));
					 });
			 }
		 };
	 },
	 [&](FFocusChanged& action) -> FDungeonReducerResult
	 {
		 Model.InteractionContext.Get<FUnitMenu>().focusedAbilityName = action.focusName;
		 return Model;
	 }
	)(worldAction);
}

template <typename... TArgs>
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
				            auto&& [childModel, eff] = reducer(historyModel.CurrentState(), a, [&]
				            {
					            historyModel.AddRecord(historyModel.CurrentState());
				            });

				            //TODO: Same kinda issue hard coding against the undo action is eh
				            if (IsInTypeSet<FBackAction>(a) && historyModel.CanGoBack())
				            {
					            return {historyModel.GoBack(), lager::noop};
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
