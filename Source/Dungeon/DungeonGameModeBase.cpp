// Copyright Epic Games, Inc. All Rights Reserved.

#include "DungeonGameModeBase.h"

#include "ADungeonWorld.h"
#include "DungeonReducer.hpp"
#include "Logic/DungeonGameState.h"
#include "SingleSubmitHandler.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/TileVisualizationActor.h"
#include "DungeonUnitActor.h"
#include "Widget/DungeonMainWidget.h"
#include "Algo/Accumulate.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/GameStateBase.h"
#include "lager/store.hpp"
#include "lager/util.hpp"
#include "lager/event_loop/manual.hpp"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"

auto localView = [&](auto model, auto... lenses)
{
	constexpr auto numberOfParams = sizeof...(lenses);
	if constexpr (numberOfParams > 1)
	{
		return lager::view(fan(lenses...), model);
	}
	else
	{
		return lager::view(lenses..., model);
	}
};

template <typename T>
struct TIsOptional
{
	enum { Value = false };
};

template <typename T>
struct TIsOptional<TOptional<T>>
{
	enum { Value = true };
};

inline FReply GenerateMoves(ADungeonGameModeBase* gm)
{
	using namespace lager::lenses;
	using namespace lager;

	const FDungeonWorldState& initialState = **gm->store;
	auto aiTeamId = initialState.TurnState.teamId;

	TMap<int, TSet<int>> TeamIdToUnitIds;

	for (TTuple<int, FDungeonLogicUnit> LoadedUnit : initialState.Map.LoadedUnits)
	{
		int UnitTeamId = LoadedUnit.Value.teamId;
		auto& TeamSet = TeamIdToUnitIds.FindOrAdd(UnitTeamId);
		TeamSet.Add(LoadedUnit.Key);
	}

	auto aiUnitIds = TeamIdToUnitIds.FindAndRemoveChecked(aiTeamId);
	auto enemyIds = Algo::TransformAccumulate(TeamIdToUnitIds, [](auto& x) { return x.Value; }, TSet<int>(),
	                                          [](auto&& acc, auto&& y)
	                                          {
		                                          acc.Append(y);
		                                          return acc;
	                                          });

	using TClosestDistanceAndUnitId = TTuple<int, int>;

	//What the fuck
	TArray<TArray<TClosestDistanceAndUnitId>> closestUnitMap = TArray<TArray<TClosestDistanceAndUnitId>>();
	auto rowInit = TArray<TClosestDistanceAndUnitId>();
	rowInit.Init(TClosestDistanceAndUnitId(), initialState.Map.Height);
	closestUnitMap.Init(rowInit, initialState.Map.Height);

	for (int i = 0; i < initialState.Map.Width; i++)
	{
		for (int j = 0; j < initialState.Map.Height; j++)
		{
			auto thisPt = FIntPoint{i, j};
			TClosestDistanceAndUnitId minTuple = {INT_MAX, -1};

			for (int enemyId : enemyIds)
			{
				auto currentTuple = TClosestDistanceAndUnitId{
					manhattanDistance(thisPt - localView(initialState, unitIdToPosition(enemyId))), enemyId
				};
				minTuple = [](auto&& a, auto&& b) { return TLess<>{}(get<0>(a), get<0>(b)); }(currentTuple, minTuple)
					           ? currentTuple
					           : minTuple;
			}

			closestUnitMap[i][j] = minTuple;
		}
	}

	auto futureAccumulator = [gm, closestUnitMap]
	(lager::future&& futureChain, int aiUnitId) -> lager::future
	{
		return MoveTemp(futureChain).then(
			[aiUnitId, closestUnitMap, gm]
			{
				using TActionsTuple = TTuple<FMoveAction, TOptional<FCombatAction>>;
				const FDungeonWorldState& state = **gm->store;
				TArray<TActionsTuple> actions;

				auto [aiPosition,aiUnitData] = lager::view(fan(unitIdToPosition(aiUnitId), unitDataLens(aiUnitId)),
				                                           state);

				auto interactablePoints = manhattanReachablePoints(
					state.Map.Width - 1,
					state.Map.Height - 1,
					aiUnitData->Movement,
					aiPosition);

				for (FIntPoint movePoint : interactablePoints)
				{
					auto possibleUnit = view(getUnitAtPointLens(movePoint), state);
					if (possibleUnit.IsSet() && *possibleUnit != aiUnitId)
					{
						continue;
					}
					auto zaTup = TActionsTuple(FMoveAction(aiUnitId, movePoint), TOptional<FCombatAction>());
					auto& [moveA,optCombat] = zaTup;
					const auto& [distance, unitId] = closestUnitMap[movePoint.X][movePoint.Y];
					auto possibleAttackUnit = localView(
						state, unitDataLens(unitId) | unreal_map_opt(attr(&FDungeonLogicUnit::Id)));

					if (distance <= aiUnitData->attackRange && possibleAttackUnit)
					{
						optCombat = FCombatAction(aiUnitId, *possibleAttackUnit, aiUnitData->damage);
					}
					actions.Add(zaTup);
				}

				Algo::Sort(actions, [&](TActionsTuple& a, TActionsTuple& b)
				{
					const auto& aPoint = a.Key.Destination;
					const auto& bPoint = b.Key.Destination;

					if (TEqualTo()(a.Value.IsSet(), b.Value.IsSet()))
					{
						if (TEqualTo()(closestUnitMap[aPoint.X][aPoint.Y].Key, closestUnitMap[bPoint.X][bPoint.Y].Key))
							return TLess<>()((aiPosition - a.Key.Destination).Size(),
							                 (aiPosition - b.Key.Destination).Size());
						else
							return TLess<>()(closestUnitMap[aPoint.X][aPoint.Y].Key,
							                 closestUnitMap[bPoint.X][bPoint.Y].Key);
					}
					else
					{
						return !TLess<>{}(a.Value.IsSet(), b.Value.IsSet());
					}
				});

				auto combatToDisp = TSet<FIntPoint>{};
				auto moveToDisp = TSet<FIntPoint>{};

				for (TActionsTuple Action : actions)
				{
					auto& [moveAct, combatAct] = Action;

					// if ([](TActionsTuple x) { return x.Value.IsSet(); }(Action))
					if (combatAct.IsSet())
					{
						combatToDisp.Add(moveAct.Destination);
						// ([](TActionsTuple x) { return x.Key.Destination; }(Action));
					}
					else
					{
						moveToDisp.Add(moveAct.Destination);
					}
				}

				// gm->TileVisualizationComponent->Clear();
				// gm->TileVisualizationComponent->ShowTiles(combatToDisp, FLinearColor::Green);
				// gm->TileVisualizationComponent->ShowTiles(moveToDisp, FLinearColor::Yellow);

				auto runThis = [gm](TActionsTuple&& tuple)
				{
					auto moveAction = tuple.Key;
					TOptional<FCombatAction> possibleCombat = tuple.Value;
					return gm->Dispatch(MoveTemp(moveAction)).then(
						[possibleCombat, id = moveAction.InitiatorId, gm]() -> lager::future
						{
							if (possibleCombat.IsSet())
							{
								return gm->Dispatch(FCombatAction(*possibleCombat)).then([=]
								{
									return gm->Dispatch(FWaitAction{id});
								});
							}

							return gm->Dispatch(FWaitAction{id});
						});
				};

				return runThis(static_cast<TActionsTuple>(actions[0]));
			});
	};

	Algo::Accumulate(aiUnitIds, lager::future(), futureAccumulator).then([gm]
	{
		gm->Dispatch(FEndTurnAction{});
	});

	return FReply::Handled();
}

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UDungeonMainWidget> HandlerClass(
		TEXT("/Game/Blueprints/Widgets/MainWidget"));
	static ConstructorHelpers::FClassFinder<AStaticMeshActor> BlockingBorderActor(
		TEXT("/Game/untitled_category/untitled_asset/BlockingBorderActor"));

	MainWidgetClass = HandlerClass.Class;
	BlockingBorderActorClass = BlockingBorderActor.Class;

	PrimaryActorTick.bCanEverTick = true;
	UnitTable = ObjectInitializer.CreateDefaultSubobject<UDataTable>(this, "Default");
	UnitTable->RowStruct = FDungeonLogicUnit::StaticStruct();
}

template <typename Fn>
auto TapReducer(Fn&& effectFunction)
{
	return [&](auto next)
	{
		return [&,next](auto action,
		                auto&& model,
		                auto&& reducer,
		                auto&& loop,
		                auto&& deps,
		                auto&& tags)
		{
			return next(action,
			            LAGER_FWD(model),
			            [reducer, effectFunction](auto m, auto a) -> decltype(reducer(LAGER_FWD(m), LAGER_FWD(a)))
			            {
				            auto&& [updatedModel, oldEffect] = reducer(LAGER_FWD(m), LAGER_FWD(a));
				            return {
					            updatedModel,
					            [ DUNGEON_MOVE_LAMBDA(oldEffect), DUNGEON_MOVE_LAMBDA(a), effectFunction ]
				            (auto&& ctx)
					            {
						            if constexpr (std::is_same_v<void,
						                                         decltype(oldEffect(ctx))>)
						            {
							            oldEffect(DUNGEON_FOWARD(ctx));
							            effectFunction(a);
						            }
						            else
						            {
							            auto&& daFuture = oldEffect(DUNGEON_FOWARD(ctx));
							            effectFunction(a);
							            return daFuture;
						            }
					            }
				            };
			            },
			            LAGER_FWD(loop),
			            LAGER_FWD(deps),
			            LAGER_FWD(tags));
		};
	};
}

void ADungeonGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	const auto TapUpdateViewingModel = [this](auto next)
	{
		return [this,next](auto action,
		                   auto&& model,
		                   auto&& reducer,
		                   auto&& loop,
		                   auto&& deps,
		                   auto&& tags)
		{
			return next(action,
			            LAGER_FWD(model),
			            [reducer, this](auto&& m, auto&& a) -> decltype(reducer(LAGER_FWD(m), LAGER_FWD(a)))
			            {
				            auto&& [updatedModel, oldEffect] = reducer(LAGER_FWD(m), LAGER_FWD(a));
				            this->ViewingModel->currentModel = updatedModel;
				            return {updatedModel, oldEffect};
			            },
			            LAGER_FWD(loop),
			            LAGER_FWD(deps),
			            LAGER_FWD(tags));
		};
	};

	TSubclassOf<ADungeonWorld> SubclassOf = ADungeonWorld::StaticClass();

	auto world = FindAllActorsOfType(GetWorld(), SubclassOf);
	world->currentWorldState.TurnState.teamId = 1;
	world->currentWorldState.Config = {.ControllerTypeMapping = {Player, Computer}};
	world->currentWorldState.InteractionContext.Set<FSelectingUnitContext>({});
	for (auto& IdToActor : world->currentWorldState.Map.LoadedUnits)
	{
		IdToActor.Value.HitPointsTotal = IdToActor.Value.HitPoints;
	}

	store = MakeUnique<FDungeonStore>(FDungeonStore(lager::make_store<TStoreAction>(
		// FHistoryModel(Game)
		FHistoryModel(world->currentWorldState)
		, lager::with_manual_event_loop{}
		, lager::with_deps(std::ref(*GetWorld()))
		// lager::with_queue_event_loop{QueueEventLoop},
		, WithUndoReducer(WorldStateReducer)
		, TapReducer([&](TDungeonAction action)
		{
			this->DungeonActionDispatched.Broadcast(action);
			// Visit([&]<typename T0>(T0 x)
			// {
			// 	using TVisitType = typename TDecay<T0>::Type;
			// 	if constexpr (TIsInTypeUnion<TVisitType, FMoveAction, FCombatAction>::Value)
			// 	{
			// 		FString outJson;
			// 		FJsonObjectConverter::UStructToJsonObjectString<T0>(x, outJson);
			// 		loggingStrings.Add(outJson);
			// 	}
			// }, action);
		})
		// ,TapUpdateViewingModel
		, lager::with_futures
	)));

	for (auto UnitIdToActor : static_cast<const FDungeonWorldState>(store->get()).unitIdToActor)
	{
		UnitIdToActor.Value->HookIntoStore(*store, this->DungeonActionDispatched);
	}

	interactionReader = store->zoom(interactionContextLens)
	                         .map([](const TInteractionContext& Context)
	                         {
		                         return Context.GetIndex();
	                         }).make();
	interactionReader.watch([this](SIZE_T typeIndex)
	{
		if (TInteractionContext(TInPlaceType<FControlledInteraction>{}).GetIndex() == typeIndex)
		{
			GenerateMoves(this);
		}
	});

	FActorSpawnParameters params;
	params.Name = FName("BoardVisualizationActor");
	auto tileShowPrefab = GetWorld()->SpawnActor<ATileVisualizationActor>(TileShowPrefab, FVector::ZeroVector,
	                                                                      FRotator::ZeroRotator, params);

	MainWidget = CreateWidget<UDungeonMainWidget>(this->GetWorld()->GetFirstPlayerController(), MainWidgetClass);
	MainWidget->AddToViewport();

	auto InAttribute = FCoreStyle::GetDefaultFontStyle("Bold", 30);
	InAttribute.OutlineSettings.OutlineSize = 3.0;
	style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
	style.SetColorAndOpacity(FSlateColor(FLinearColor::White));

	auto unitPropertyToTextLambda = [&](auto&& getter)
	{
		return [this,getter]
		{
			FText valueToDisplay;
			const TOptional<FDungeonLogicUnit>& unitUnderCursor = lager::view(getUnitUnderCursor, store->get());

			if (!unitUnderCursor.IsSet())
				return valueToDisplay;

			if constexpr (TIsMemberPointer<typename TDecay<decltype(getter)>::Type>::Value)
			{
				valueToDisplay = FCoerceToFText::Value(unitUnderCursor.GetValue().*getter);
			}
			else
			{
				valueToDisplay = FCoerceToFText::Value(getter(unitUnderCursor.GetValue()));
			}

			return valueToDisplay;
		};
	};

	auto AttributeDisplay = [&](FString&& AttributeName, auto&& lambda) -> decltype(SVerticalBox::Slot())
	{
		return MoveTemp(SVerticalBox::Slot().AutoHeight()[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
           .TextStyle(&style)
           .Font(InAttribute)
           .Text(FText::FromString(DUNGEON_FOWARD(AttributeName)))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
           .TextStyle(&style)
           .Font(InAttribute)
           .Text_Lambda(DUNGEON_FOWARD(lambda))
			]
		]);
	};

	auto getTextReaderForLens = [this](auto&& lens)
	{
		return store->zoom(lens).map([](auto&& unitUnderCursor) -> FText
		{
			if constexpr (TNot<TIsOptional<TDecay<decltype(unitUnderCursor)>::Type>>::Value)
				return FCoerceToFText::Value(unitUnderCursor);
			else
			{
				if (!unitUnderCursor.IsSet())
					return {};

				return FCoerceToFText::Value(*unitUnderCursor);
			}
		}).make();
	};

	MainWidget->UnitDisplay->SetContent(
		SNew(SVerticalBox)
		+ AttributeDisplay(
			"Name", getTextReaderForLens(getUnitUnderCursor | unreal_map_opt(attr(&FDungeonLogicUnit::Name))))
		+ AttributeDisplay("HitPoints", unitPropertyToTextLambda(&FDungeonLogicUnit::HitPoints))
		+ AttributeDisplay("Movement", unitPropertyToTextLambda(&FDungeonLogicUnit::Movement))
		+ AttributeDisplay("Turn ID", getTextReaderForLens(
			                   SimpleCastTo<FDungeonWorldState> | attr(&FDungeonWorldState::TurnState) | attr(
				                   &FTurnState::teamId)))
		+ AttributeDisplay("Context Interaction", [&]() -> auto
		{
			return Visit([&](auto&& context)
			{
				return FText::FromString(
					TDecay<decltype(context)>::Type::StaticStruct()->GetName());
			}, UseViewState(interactionContextLens));
		})
	);

	this->SingleSubmitHandler = static_cast<USingleSubmitHandler*>(this->AddComponentByClass(
		USingleSubmitHandler::StaticClass(), false, FTransform(), false));
}

void ADungeonGameModeBase::Tick(float deltaTime)
{
	Super::Tick(deltaTime);
}

lager::future ADungeonGameModeBase::Dispatch(TDungeonAction&& unionAction)
{
	return store->dispatch(MoveTemp(unionAction));
}
