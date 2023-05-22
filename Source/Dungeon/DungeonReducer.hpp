#pragma once
#include "Animation/AnimInstance.h"
#include "Dungeon.h"
#include "DungeonConstants.h"
#include "DungeonUnitActor.h"
#include "GraphAStar.h"
#include "PromiseFulfiller.h"
#include "Algo/Copy.h"
#include "Algo/ForEach.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Logic/DungeonGameState.h"
#include "lager/store.hpp"
#include "lager/util.hpp"
#include "lager/lenses/tuple.hpp"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"
#include "Logic/util.h"
#include "Misc/StringFormatter.h"
#include "Utility/Macros.hpp"
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
	template <typename T>
	T operator()(const T& t)
	{
		return t;
	}

	TRANSITION_EDGE_FROM_TO(FSelectingUnitContext, FMainMenu)
	TRANSITION_EDGE_FROM_TO(FMainMenu, FSelectingUnitContext)
	TRANSITION_EDGE_FROM_TO(FUnitMenu, FSelectingUnitContext)
	TRANSITION_EDGE_FROM_TO(FSelectingUnitAbilityTarget, FUnitMenu)
	TRANSITION_EDGE_FROM_TO(FUnitInteraction, FSelectingUnitAbilityTarget)
};

//UHHMMM GROSS DEPARTMENT?
template <typename TDelegate, bool TReturnTuple = false>
inline decltype(auto) CreateResolvingFutureOnEventRaise(TDelegate& OnInterpToStopDelegate, const auto& context,
                                                        typename TDelegate::FDelegate::template TMethodPtrResolver<
	                                                        UPromiseFulfiller>::FMethodPtr FuncPtr,
                                                        FName InFunctionName)
{
	auto PromiseFulfiller = NewObject<UPromiseFulfiller>();
	PromiseFulfiller->AddToRoot();
	auto [p,f] = lager::promise::with_loop(context.loop());
	PromiseFulfiller->promiseToFulfill = MoveTemp(p);
	OnInterpToStopDelegate.__Internal_AddUniqueDynamic(PromiseFulfiller, FuncPtr, InFunctionName);
	auto chainedFuture = MoveTemp(f).then(
		[&OnInterpToStopDelegate, PromiseFulfiller, FuncPtr, InFunctionName ]
		{
			OnInterpToStopDelegate.__Internal_RemoveDynamic(PromiseFulfiller, FuncPtr, InFunctionName);
		});

	if constexpr (TReturnTuple)
		return MakeTuple(PromiseFulfiller, MoveTemp(chainedFuture));
	else
		return [chainedFuture, PromiseFulfiller](auto&& inBetweenFuture) mutable -> lager::future
		{
			return MoveTemp(chainedFuture).then(DUNGEON_FOWARD(inBetweenFuture)).then([PromiseFulfiller]
			{
				PromiseFulfiller->RemoveFromRoot();
			});
		};
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

const auto isNull = [](auto&& ptr)
{
	return ptr == nullptr;
};

const auto isAll = [](auto&& theCheck, auto&&... values)
{
	return ( theCheck(DUNGEON_FOWARD(values)) && ... );
};

const auto anyOf = [](auto&& theCheck, auto&&... values)
{
	return ( theCheck(DUNGEON_FOWARD(values)) || ... );
};

const auto identity = []
{
};

const auto GetConfig = lager::lenses::attr(&FDungeonWorldState::Config);

inline int GetNumberOfPlayers(const FDungeonWorldState& Model)
{
	return lager::view(GetConfig | attr(&FConfig::ControllerTypeMapping), Model).Num();
}

inline TInteractionContext GetStartingContextForControllerType(EPlayerType Controller)
{
	switch (Controller)
	{
	case Computer:
		return TInteractionContext(TInPlaceType<FControlledInteraction>{});
	case Player:
		return TInteractionContext(TInPlaceType<FSelectingUnitContext>{});
	default:
		throw "shit that ain't gonna work";
	}
}

inline auto WorldStateReducer(FDungeonWorldState Model, TDungeonAction worldAction,
                              TFunctionRef<void()>&& signalCheckpoint = identity) -> FDungeonReducerResult
{
	const auto ReducerView = [&Model](auto&& Lens) { return lager::view(DUNGEON_FOWARD(Lens), Model); };
	const auto DefaultPassthrough = [&](auto& x) -> FDungeonReducerResult { return {Model, lager::noop}; };
	const auto Matcher = Dungeon::Match(DefaultPassthrough);

	return Matcher
	(
		[&](FBackAction&) -> FDungeonReducerResult
		{
			return Dungeon::MatchAgnostic(
				[&](auto& all) -> FDungeonReducerResult
				{
					auto Value = FBackTransitions{}(all);
					Model.InteractionContext.Set<decltype(Value)>(Value);
					return Model;
				}
			)(Model.InteractionContext);
		},
		[&](FChangeTeam& action) -> FDungeonReducerResult
		{
			Model.TurnState.teamId = action.newTeamID;
			return Model;
		},
		[&](FAttachLogicToDisplayUnit& action) -> FDungeonReducerResult
		{
			Model.unitIdToActor.Add(action.Id, action.actor);
			return Model;
		},
		[&](FSpawnUnit& action) -> FDungeonReducerResult
		{
			const auto& Lens = lager::lenses::fan(
				unitDataLens(action.Unit.Id),
				getUnitAtPointLens(action.Position));
			auto UpdatedModel = lager::set(Lens, Model,
			                               std::make_tuple(TOptional(action.Unit), TOptional(action.Unit.Id)));
			return {
				UpdatedModel,
				[unitId = action.Unit.Id, action,prefabClass = action.PrefabClass](const FDungeonStore::context_t& ctx)
				{
					UWorld& world = get<UWorld&>(ctx);
					ADungeonUnitActor* actor = world.SpawnActor<ADungeonUnitActor>(
						LoadObject<UBlueprint>(nullptr, *prefabClass)->GeneratedClass);

					actor->SetActorLocation(FVector(action.Position) * TILE_POINT_SCALE + FVector{0, 0, 1.0});
					actor->Id = unitId;
					ctx.dispatch(CreateDungeonAction(FAttachLogicToDisplayUnit{.Id = unitId, .actor = actor}));
				}
			};
		},
		[&](FInteractionResults& action) -> FDungeonReducerResult
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
						//TODO: eh gross I'm not a big fan of this api
						ctx.dispatch(
							TDungeonAction(TInPlaceType<FChangeState>{},
							               TInteractionContext(TInPlaceType<FSelectingUnitContext>{})));
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

					  waitingAction.Destination = actionTarget.target;

					  return {
						  Model,
						  [DUNGEON_MOVE_LAMBDA(waitingAction = MoveTemp(waitingAction)), signalCheckpoint, Model](
						  auto ctx)
						  {
							  ctx.dispatch(CreateDungeonAction(FRecordAction{Model}));
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

						  //So there's an interaction left to resolve due to this being combat...
						  FUnitInteraction* UnitInteraction = Model.InteractionsToResolve.Top().
						                                            TryGet<FUnitInteraction>();
						  UnitInteraction->originatorID = waitingAction.InitiatorId;
						  TOptional<int> possibleTarget = ReducerView(
							  getUnitAtPointLens(
								  ReducerView(cursorPositionLens)));
						  UnitInteraction->targetIDUnderFocus = *possibleTarget;

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

				 return Model;
			 }
			)(Model.InteractionContext);
		},
		[&](FSteppedAction& action) -> FDungeonReducerResult
		{
			Model.InteractionsToResolve = action.interactions;
			Model.WaitingForResolution = action.mainAction;
			signalCheckpoint();
			return {
				Model, [=](const auto& ctx)
				{
					ctx.dispatch(TDungeonAction(TInPlaceType<FChangeState>{}, action.interactions.Top()));
				}
			};
		},
		[&](FChangeState& action) -> FDungeonReducerResult
		{
			return Visit(lager::visitor{
				             [&Model](auto& typedContext) -> FDungeonReducerResult
				             {
					             using TVisitType = typename TDecay<decltype(typedContext)>::Type;
					             Model.InteractionContext.Set<TVisitType>(typedContext);
					             return Model;
				             },
				             [&ReducerView, &Model](FUnitInteraction& InteractionContext) -> FDungeonReducerResult
				             {
					             int targetID = ReducerView(
						             getUnitAtPointLens(ReducerView(cursorPositionLens)) | ignoreOptional);
					             InteractionContext.targetIDUnderFocus = targetID;
					             Model.InteractionContext.Set<FUnitInteraction>(InteractionContext);
					             TWeakObjectPtr<ADungeonUnitActor> ActorPtr = ReducerView(
						             unitIdToActor(InteractionContext.originatorID));

					             return {
						             Model, [ActorPtr](auto& ctx)
						             {
							             if (UAnimInstance* AnimInstance = ActorPtr->FindComponentByClass<
									             USkeletalMeshComponent>()
								             ->GetAnimInstance())
							             {
								             if (ActorPtr->CombatActionAnimationWindUp)
								             {
									             AnimInstance->PlaySlotAnimationAsDynamicMontage(
										             ActorPtr->CombatActionAnimationWindUp, "UpperBody", .25, .25, 1.,
										             9999999.);
								             }
							             }
						             }
					             };
				             },
			             }, action.newState);
		},
		[&](FEndTurnAction& action) -> FDungeonReducerResult
		{
			const int MAX_PLAYERS = GetNumberOfPlayers(Model);
			const auto nextTeamId = (Model.TurnState.teamId % MAX_PLAYERS) + 1;

			Model.TurnState = {nextTeamId};
			const auto& Controllers = ReducerView(GetConfig | attr(&FConfig::ControllerTypeMapping));
			const auto& ControllerType = Controllers[nextTeamId - 1];
			Model.InteractionContext = GetStartingContextForControllerType(ControllerType);
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
			(const FDungeonStore::context_t& context)
				{
					if (!actor.IsValid()) //TODO Fuck this runs in unit tests
						return lager::future{};

					auto actorPtr = actor;
					auto moveComp = actorPtr->InterpToMovementComponent;
					moveComp->ResetControlPoints();

					FIntPoint UpdatedPosition = MoveAction.Destination;

					TArray<FIntPoint> aStarResult;
					FSimpleTileGraph SimpleTileGraph = FSimpleTileGraph(DungeonLogicMap, DungeonLogicUnit.Movement);
					FGraphAStar aStarGraph(SimpleTileGraph);

					//TODO: there should always be a result?
					aStarGraph.FindPath(LastPosition, UpdatedPosition, SimpleTileGraph, aStarResult);

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

					auto fut = CreateFutureFromUnrealDelegate(moveComp->OnInterpToStop, context,
					                                          &UPromiseFulfiller::HandleOnInterpToStop)
					([springArm, actorPtr, oldParent]
					{
						UKismetSystemLibrary::PrintString(actorPtr.Get(),
						                                  FString::Format(
							                                  TEXT("ON HANDLE INTERP TO STOP CALLED"), {1}));

						springArm->AttachToComponent(
							oldParent, FAttachmentTransformRules(EAttachmentRule::KeepRelative, false));
					});
					
					return MoveTemp(fut);
				}
			};
		},
		[&](FKillUnitAction& action) -> FDungeonReducerResult
		{
			auto actor = Model.unitIdToActor.FindAndRemoveChecked(action.unitID);
			actor->Destroy();
			Model.Map.LoadedUnits.Remove(action.unitID);
			auto foundID = *Model.Map.UnitAssignment.FindKey(action.unitID);
			Model.Map.UnitAssignment.Remove(foundID);

			return Model;
		},
		[&](FCombatAction& action) -> FDungeonReducerResult
		{
			FDungeonLogicUnit& initiatorUnit = Model.Map.LoadedUnits.FindChecked(action.InitiatorId);
			TMap<int, TWeakObjectPtr<ADungeonUnitActor>> actorMap = ReducerView(
				attr(&FDungeonWorldState::unitIdToActor));
			TWeakObjectPtr<ADungeonUnitActor> initatorActor = zug::comp(ReducerView, unitIdToActor)(action.InitiatorId);

			TOptional<FKillUnitAction> maybeKill;

			// zug::comp(ReducerView, unitIdToData)(action.targetedUnit)

			Model.TurnState.unitsFinished.Add(initiatorUnit.Id);

			Model = lager::over(unitIdToData(action.targetedUnit), Model, [&](FDungeonLogicUnit Unit)
			{
				if (Unit.HitPoints - action.damageValue <= 0)
				{
					maybeKill = FKillUnitAction{Unit.Id};
				}

				Unit.HitPoints -= action.damageValue;
				return Unit;
			});

			signalCheckpoint();

			return {
				Model, [initatorActor, maybeKill](const FDungeonStore::context_t& ctx)
				{
					UAnimInstance* AnimInstance = initatorActor
					                              ->FindComponentByClass<USkeletalMeshComponent>()
					                              ->GetAnimInstance();
					if (anyOf(isNull, AnimInstance, initatorActor->CombatActionAnimationSubmit))
					{
						return lager::future();
					}

					// UKismetSystemLibrary::PrintString(AnimInstance, FString::Format(TEXT( "OK START TIME: {0}" ), { FDateTime::Now().GetTicks() }));
					
					UAnimMontage* Montage = AnimInstance->PlaySlotAnimationAsDynamicMontage(
						initatorActor->CombatActionAnimationSubmit,
						"UpperBody", .25, .25, .5);

					auto [promise, fut] = lager::promise::with_loop(ctx.loop());

					//TODO: hmm this seems to be for a montage ending but not the blending out HMM
					AnimInstance->QueueMontageEndedEvent(FQueuedMontageEndedEvent(Montage, false,
						FOnMontageEnded::CreateLambda([ctx, maybeKill, promise = MoveTemp(promise)](...) mutable
						{
							if (maybeKill)
							{
								ctx.dispatch(CreateDungeonAction(FKillUnitAction(maybeKill.GetValue())));
							}

							ctx.dispatch(TDungeonAction(TInPlaceType<FCommitAction>{}));
							
							// UKismetSystemLibrary::PrintString(AnimInstance, FString::Format(TEXT( "OK START TIME: {0}" ), { FDateTime::Now().GetTicks() }));
							promise();
						})));

					return fut;
				}
			};
		},
		[&](FWaitAction& action) -> FDungeonReducerResult
		{
			Model.TurnState.unitsFinished.Add(action.InitiatorId);

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

				            if (IsInTypeSet<FRecordAction>(a))
				            {
					            historyModel.AddRecord(a.Get<FRecordAction>().Record);
				            }

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
