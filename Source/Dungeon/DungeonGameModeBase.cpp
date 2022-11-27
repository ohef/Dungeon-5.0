// Copyright Epic Games, Inc. All Rights Reserved.

#include "DungeonGameModeBase.h"

#include "Logic/DungeonGameState.h"
#include "SingleSubmitHandler.h"
#include "Actions/EndTurnAction.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Actor/TileVisualizationActor.h"
#include "DungeonUnitActor.h"
#include "GraphAStar.h"
#include "Algo/Copy.h"
#include "Algo/FindLast.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameStateBase.h"
#include "immer/map.hpp"
#include "Kismet/KismetSystemLibrary.h"
#include "lager/event_loop/manual.hpp"
#include "lager/store.hpp"
#include "lager/util.hpp"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"
#include "Logic/util.h"
#include "Serialization/Csv/CsvParser.h"
#include "State/SelectingGameState.h"
#include "Utility/VariantVisitor.hpp"

ADungeonGameModeBase::ADungeonGameModeBase(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UDungeonMainWidget> HandlerClass(
		TEXT("/Game/Blueprints/Widgets/MainWidget"));
	MainWidgetClass = HandlerClass.Class;

	PrimaryActorTick.bCanEverTick = true;
	UnitTable = ObjectInitializer.CreateDefaultSubobject<UDataTable>(this, "Default");
	UnitTable->RowStruct = FDungeonLogicUnit::StaticStruct();
}

#define TRANSITION_EDGE_FROM_TO(TFrom, TTo) \
  TTo operator()(TFrom& from) \
  {\
    return TTo();\
  }

auto GetInteractionFields(FDungeonWorldState Model, int unitIdUnderCursor) -> TTuple<TSet<FIntPoint>, TSet<FIntPoint>>
{
	auto foundUnitLogic = *lager::view(unitDataLens(unitIdUnderCursor), Model);
	auto unitsPosition = lager::view(unitIdToPosition(unitIdUnderCursor), Model);

	TSet<FIntPoint> points;

	bool unitCanTakeAction = !lager::view(isUnitFinishedLens2(unitIdUnderCursor), Model).IsSet();
	int interactionDistance = foundUnitLogic.Movement + foundUnitLogic.attackRange;

	if (unitCanTakeAction)
	{
		points = manhattanReachablePoints(
			Model.map.Width,
			Model.map.Height,
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

auto WorldStateReducer(FDungeonWorldState Model, TDungeonAction worldAction) -> FDungeonReducerResult
{
	const auto DungeonView = [&Model](auto&& Lens) { return lager::view(DUNGEON_FOWARD(Lens), Model); };

	return Visit(TDungeonVisitor
	             {
		             [&](FBackAction&) -> FDungeonReducerResult
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
		             [&](FInteractionAction& action) -> FDungeonReducerResult
		             {
			             return Visit(TDungeonVisitor{
				                          [&](FCombatAction& waitingAction) -> FDungeonReducerResult
				                          {
					                          auto damage = DungeonView(unitDataLens(waitingAction.InitiatorId))->damage
						                          - floor((0.05 * action[0].order));

					                          waitingAction.updatedUnit.HitPoints -= damage;
					                          waitingAction.damageValue = damage;

					                          Model.InteractionsToResolve.Pop();

					                          return {Model, [waitingAction = MoveTemp(waitingAction)](auto ctx)
					                          {
					                          	ctx.dispatch(TDungeonAction(TInPlaceType<FCombatAction>{}, waitingAction));
					                          }};
				                          },
				                          [&](auto) -> FDungeonReducerResult
				                          {
					                          return {Model, lager::noop};
				                          }
			                          }, Model.WaitingForResolution);
		             },
		             [&](FCursorQueryTarget& actionTarget) -> FDungeonReducerResult
		             {
			             return Visit(TDungeonVisitor{
				             [&](FSelectingUnitAbilityTarget&) -> FDungeonReducerResult
				             {
					             return Visit(TDungeonVisitor{
						                          [&](FMoveAction waitingAction) -> FDungeonReducerResult
						                          {
							                          bool bIsUnitThere = DungeonView(
								                          getUnitAtPointLens(actionTarget.target)).IsSet();
							                          auto [movementTiles, attackTiles] = GetInteractionFields(
								                          Model, waitingAction.InitiatorId);
							                          if (bIsUnitThere || !movementTiles.Contains(actionTarget.target))
								                          return {Model, lager::noop};

							                          waitingAction.Destination = actionTarget.target;
							                          Model.InteractionsToResolve.Pop();
							                          Model.InteractionContext.Set<FSelectingUnitContext>({});
							                          return {Model,
								                          [waitingAction = MoveTemp(waitingAction)](auto ctx)
								                          {
									                          ctx.dispatch(TDungeonAction(
										                          TInPlaceType<FMoveAction>{}, waitingAction));
								                          }
							                          };
						                          },
						                          [&](FCombatAction& waitingAction) -> FDungeonReducerResult
						                          {
							                          TOptional<FDungeonLogicUnit> initiatingUnit = DungeonView(
								                          unitDataLens(waitingAction.InitiatorId));
							                          int targetedUnitId = DungeonView(
								                          getUnitAtPointLens(actionTarget.target) | unreal_value_or(-1));
							                          TOptional<FDungeonLogicUnit> targetedUnit = DungeonView(unitDataLens(targetedUnitId));

							                          auto [movementTiles, attackTiles] = GetInteractionFields(
								                          Model, waitingAction.InitiatorId);

							                          bool isInRange = attackTiles.Union(movementTiles).Contains(actionTarget.target);
							                          bool isUnitThere = targetedUnit.IsSet();
							                          bool isTargetNotOnTheSameTeam = targetedUnit.IsSet() && targetedUnit->teamId != initiatingUnit->teamId;
							                          if (isInRange && isUnitThere && isTargetNotOnTheSameTeam)
							                          {
								                          waitingAction.target = actionTarget.target;
								                          waitingAction.updatedUnit = targetedUnit.GetValue();
								                          Model.InteractionsToResolve.Pop();
								                          Model.InteractionContext = Model.InteractionsToResolve.Top();
							                          }

							                          // //move unit, if applicable...
							                          // auto result = [&]( int range, int maxPathLength, TArray<FIntPoint> suggestedMovement, FIntPoint currentPosition, FIntPoint target)
							                          // {
							                          //   auto suggestedAttackingPoint = suggestedMovement.
							                          //    Last();
							                          //   auto attackableTiles = attackTiles;
							                          //   if (attackableTiles.Contains(suggestedAttackingPoint))
							                          //    return suggestedMovement;
							                          //
							                          //   int solutionLength = 999999;
							                          //   auto output = Algo::Accumulate( attackableTiles, TArray<FIntPoint>(),
							                          //    [&](auto acc, auto val)
							                          //    {
							                          //     TArray<FIntPoint> aStarResult;
							                          //     FSimpleTileGraph SimpleTileGraph = FSimpleTileGraph( Model.map, maxPathLength);
							                          //     FGraphAStar aStarGraph(SimpleTileGraph);
							                          //     aStarGraph.FindPath( currentPosition, target, SimpleTileGraph, aStarResult);
							                          //
							                          //     if (solutionLength > aStarResult.Num())
							                          //     {
							                          //      solutionLength = aStarResult.Num();
							                          //      return aStarResult;
							                          //     }
							                          //     else
							                          //     {
							                          //      return acc;
							                          //     }
							                          //    });
							                          //
							                          //   return output;
							                          //  }(initiatingUnit->attackRange, initiatingUnit->Movement,
							                          //    {{-1, -1}}, actionTarget.target, pt);
						                          	
							                          return {Model, lager::noop};
						                          },
						                          [&](auto) -> FDungeonReducerResult
						                          {
							                          return {Model, lager::noop};
						                          }
					                          }, Model.WaitingForResolution);
				             },
				             [&](FSelectingUnitContext&) -> FDungeonReducerResult
				             {
					             auto foundUnitOpt = DungeonView(interactionContextLens
						                                 | unreal_alternative_pipeline_t<FSelectingUnitContext>()
						                                 | map_opt(lager::lenses::attr(
							                                 &FSelectingUnitContext::unitUnderCursor))
						                                 | or_default) ;

					             if (!foundUnitOpt.IsSet())
						             return {Model, lager::noop};

					             auto foundUnitId = foundUnitOpt.GetValue();
					             auto isFinished = !DungeonView(isUnitFinishedLens2(foundUnitId)).IsSet();

					             const auto& TurnState = DungeonView(attr(&FDungeonWorldState::TurnState));
					             auto DungeonLogicUnit = DungeonView(unitDataLens(foundUnitId) | ignoreOptional);

					             auto isOnTeam = TurnState.teamId == DungeonLogicUnit.teamId;
					             if (isFinished && isOnTeam)
					             {
						             return {
							             Model, [foundUnitId](auto& ctx)
							             {
								             ctx.dispatch(
									             TDungeonAction(TInPlaceType<FChangeState>{},
									                            TInteractionContext(
										                            TInPlaceType<FUnitMenu>{}, foundUnitId)));
							             }
						             };
					             }
				             	else {
					             return {Model, lager::noop};
				             	}
				             },
				             [&](auto&) -> FDungeonReducerResult
				             {
					             return {Model, lager::noop};
				             }
			             }, Model.InteractionContext);
		             },
		             [&](FInteractAction& action) -> FDungeonReducerResult
		             {
			             Model.InteractionsToResolve = action.interactions;
			             Model.InteractionContext = action.interactions.Top();
			             Model.WaitingForResolution = action.mainAction;
			             return {Model, lager::noop};
		             },
		             [&](FWaitAction& action) -> FDungeonReducerResult
		             {
			             Model.TurnState.unitsFinished.Add(action.InitiatorId);
			             Model.InteractionContext.Set<FSelectingUnitContext>({});
			             return {Model, lager::noop};
		             },
		             [&](FChangeState& action) -> FDungeonReducerResult
		             {
			             Model.InteractionContext = action.newState;
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
		             [&](FCursorPositionUpdated& cpeEvent) -> FDungeonReducerResult
		             {
			             Visit(FCursorPositionUpdatedHandler(Model, cpeEvent.cursorPosition), Model.InteractionContext);
			             return {Model, lager::noop};
		             },
		             [&](auto& action) -> FDungeonReducerResult
		             {
			             using T = typename TDecay<decltype(action)>::Type;
			             if constexpr (TIsInTypeUnion<T, FCombatAction, FMoveAction, FWaitAction>::Value)
			             {
				             FDungeonLogicUnit& foundUnit = Model.map.loadedUnits.FindChecked(action.InitiatorId);
				             foundUnit.state = UnitState::ActionTaken;
				             Model.TurnState.unitsFinished.Add(foundUnit.Id);

				             lager::visitor{
					             [&](FMoveAction& MoveAction)
					             {
						             FDungeonLogicMap& DungeonLogicMap = Model.map;
						             DungeonLogicMap.unitAssignment.Remove(
							             *DungeonLogicMap.unitAssignment.FindKey(MoveAction.InitiatorId));
						             DungeonLogicMap.unitAssignment.
						                             Add(MoveAction.Destination, MoveAction.InitiatorId);
					             },
					             [&](FCombatAction& CombatAction)
					             {
						             FDungeonLogicUnit& Unit = CombatAction.updatedUnit;
						             Unit.state = DungeonView(isUnitFinishedLens2(Unit.Id)).IsSet()
							                          ? UnitState::ActionTaken
							                          : UnitState::Free;
						             Model = lager::set(unitDataLens(Unit.Id), Model, CombatAction.updatedUnit);
						             Model.InteractionContext.Set<FSelectingUnitContext>({});
					             },
					             [&](FWaitAction& WaitAction)
					             {
					             }
				             }(action);
			             }

			             return {Model, lager::noop};
		             },
	             }, worldAction);
};

auto WithGlobalEventMiddleware(const ADungeonGameModeBase& gamemode)
{
    return [&](auto next) {
        return [&,next](auto action,
                               auto&& model,
                               auto&& reducer,
                               auto&& loop,
                               auto&& deps,
                               auto&& tags) {
            return next(action,
                        LAGER_FWD(model),
                        [reducer, &gamemode](auto m, auto a) {
	                        gamemode.DungeonActionDispatched.Broadcast(a);
	                        return reducer(LAGER_FWD(m), LAGER_FWD(a));
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

	TArray<FDungeonLogicUnitRow*> unitsArray;
	UnitTable->GetAllRows(TEXT(""), unitsArray);
	TMap<int, FDungeonLogicUnitRow> loadedUnitTypes;
	for (auto unit : unitsArray)
	{
		loadedUnitTypes.Add(unit->unitData.Id, *unit);
	}

	FString StuffR;
	FString TheStr = FPaths::Combine(FPaths::ProjectDir(), TEXT("theBoard.csv"));
	if (FFileHelper::LoadFileToString(StuffR, ToCStr(TheStr)))
	{
		const FCsvParser Parser(MoveTemp(StuffR));
		const auto& Rows = Parser.GetRows();
		int32 height = Rows.Num();

		Game.map.loadedTiles.Add(1, {1, "Grass", 1});

		for (int i = 0; i < height; i++)
		{
			int32 width = Rows[i].Num();
			Game.map.Height = height;
			Game.map.Width = width;
			Game.TurnState.teamId = 1;
			for (int j = 0; j < width; j++)
			{
				int dataIndex = i * width + j;
				auto value = FCString::Atoi(Rows[i][j]);

				Game.map.tileAssignment.Add({i, j}, 1);

				if (loadedUnitTypes.Contains(value))
				{
					auto zaRow = loadedUnitTypes[value];
					auto zaunit = zaRow.unitData;
					zaunit.Id = dataIndex;
					zaunit.Name = FString::FormatAsNumber(dataIndex);
					Game.map.loadedUnits.Add(zaunit.Id, zaunit);

					FIntPoint positionPlacement{i, j};
					Game.map.unitAssignment.Add(positionPlacement, dataIndex);

					UClass* UnrealActor = zaRow.UnrealActor.Get();
					FTransform Transform = FTransform{FRotator::ZeroRotator, FVector::ZeroVector};
					auto unitActor = GetWorld()->SpawnActor<ADungeonUnitActor>(UnrealActor, Transform);
					unitActor->id = zaunit.Id;

					Game.unitIdToActor.Add(zaunit.Id, unitActor);
					unitActor->SetActorLocation(TilePositionToWorldPoint(positionPlacement));
				}
			}
		}
	}

	store = MakeUnique<lager::store<TStoreAction, TModelType>>(
		lager::make_store<TStoreAction>(
			TModelType(this->Game),
			lager::with_manual_event_loop{},
			lager::with_reducer(WorldStateReducer),
			WithGlobalEventMiddleware(*this)
		));

	for (auto UnitIdToActor : static_cast<const FDungeonWorldState>(store->get()).unitIdToActor)
	{
		UnitIdToActor.Value->hookIntoStore();
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
		FFileHelper::SaveStringToFile(
			TableAsCSV, ToCStr(FPaths::Combine(FPaths::ProjectDir(), TEXT("UnitIdsOutput.csv"))));
	}

	FActorSpawnParameters params;
	params.Name = FName("BoardVisualizationActor");
	auto tileShowPrefab = GetWorld()->SpawnActor<ATileVisualizationActor>(TileShowPrefab, FVector::ZeroVector,
	                                                                      FRotator::ZeroRotator, params);

	MovementVisualization = tileShowPrefab->MovementVisualizationComponent;
	TileVisualizationComponent = tileShowPrefab->TileVisualizationComponent;

	MainWidget = CreateWidget<UDungeonMainWidget>(this->GetWorld()->GetFirstPlayerController(), MainWidgetClass);
	MainWidget->AddToViewport();

	auto InAttribute = FCoreStyle::GetDefaultFontStyle("Bold", 30);
	InAttribute.OutlineSettings.OutlineSize = 3.0;
	style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
	style.SetColorAndOpacity(FSlateColor(FLinearColor::White));

	auto AttributeDisplay = [&](FString&& AttributeName, auto&& methodPointer) -> decltype(SVerticalBox::Slot())
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
           .Text_Lambda(DUNGEON_FOWARD(methodPointer))
			]
		]);
	};

	auto unitPropertyToText = [&](auto&& getter)
	{
		return [this,getter = DUNGEON_FOWARD(getter)]{
		const auto& unitUnderCursor = lager::view(getUnitUnderCursor, store->get());
		return unitUnderCursor.IsSet() ? FCoerceToFText::Value(getter(unitUnderCursor.GetValue())) : FText();
		};
	};
	
	MainWidget->UnitDisplay->SetContent(
		SNew(SVerticalBox)
		+ AttributeDisplay("Name", unitPropertyToText([](const FDungeonLogicUnit x){return x.Name; }))
		+ AttributeDisplay("HitPoints", unitPropertyToText([](const FDungeonLogicUnit x){return x.HitPoints; }))
		+ AttributeDisplay("Movement", unitPropertyToText([](const FDungeonLogicUnit x){return x.Movement; }))
		+ AttributeDisplay("Turn ID", [&] { return GetCurrentTurnId(); })
		+ SVerticalBox::Slot().AutoHeight()[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
                     .TextStyle(&style)
                     .Font(InAttribute)
                     .Text(FText::FromString("Context Interaction"))
			]
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
                     .TextStyle(&style)
                     .Font(InAttribute)
                     .Text_Lambda([&]()-> auto
				                {
					                return Visit([&](auto&& context)
					                {
						                return FText::FromString(
							                (TDecay<decltype(context)>::Type::StaticStruct()->GetName()));
					                }, store->get().InteractionContext);
				                })
			]
		]
	);

	this->SingleSubmitHandler = static_cast<USingleSubmitHandler*>(this->AddComponentByClass(
		USingleSubmitHandler::StaticClass(), false, FTransform(), false));
}

void ADungeonGameModeBase::GoBackOnInputState()
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

TWeakPtr<FState> ADungeonGameModeBase::GetCurrentState()
{
	return stateStack.IsEmpty() ? baseState : stateStack.Top();
}

void ADungeonGameModeBase::CommitAndGotoBaseState()
{
	GetCurrentState().Pin()->Exit();
	stateStack.Empty();
	GetCurrentState().Pin()->Enter();
}

void ADungeonGameModeBase::RefocusMenu()
{
	if (MainWidget->Move->HasUserFocus(0))
		return;
	MainWidget->Move->SetFocus();
}

AMapCursorPawn* ADungeonGameModeBase::GetMapCursorPawn()
{
	return this
	       ->GetWorld()
	       ->GetFirstPlayerController()
	       ->GetPawn<AMapCursorPawn>();
}

void ADungeonGameModeBase::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	if (this->AnimationQueue.IsEmpty())
		return;

	(*this->AnimationQueue.Peek())->TickTimeline(deltaTime);
}

void ADungeonGameModeBase::Dispatch(TDungeonAction&& unionAction)
{
	store->dispatch(unionAction);
}

FText ADungeonGameModeBase::GetCurrentTurnId() const
{
	return FCoerceToFText::Value(lager::view(turnStateLens, *this).teamId);
}
