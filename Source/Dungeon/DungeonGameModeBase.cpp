// Copyright Epic Games, Inc. All Rights Reserved.

#include "DungeonGameModeBase.h"

#include <Logic/DungeonGameState.h>

#include "Actions/EndTurnAction.h"
#include "Actor/DungeonPlayerController.h"
#include "Actor/MapCursorPawn.h"
#include "Algo/FindLast.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Lenses/model.hpp"
#include "Logic/SimpleTileGraph.h"
#include "Serialization/Csv/CsvParser.h"
#include "SingleSubmitHandler.h"
#include "Actor/TileVisualizationActor.h"
#include "Algo/Copy.h"
#include "State/SelectingGameState.h"
#include "immer/map.hpp"
#include "lager/event_loop/manual.hpp"
#include "lager/lenses/tuple.hpp"
#include "lager/store.hpp"
#include "lager/util.hpp"
#include "Logic/util.h"
#include "Utility/ContextSwitchVisitor.hpp"
#include "Utility/HookFunctor.hpp"
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
		currentContext.unitUnderCursor = lager::view(getUnitAtPointLens(cursorPosition), Model);
		currentContext.interactionTiles.Find(ETargetsAvailableId::move)->Empty();
		currentContext.interactionTiles.Find(ETargetsAvailableId::attack)->Empty();

		if(!currentContext.unitUnderCursor.IsSet())
			return;

		int unitIdUnderCursor = *currentContext.unitUnderCursor;
		auto foundUnitLogic = *lager::view( unitDataLens(unitIdUnderCursor), Model);

		TSet<FIntPoint> points;
		
		bool unitCanTakeAction = !lager::view(isUnitFinishedLens2(unitIdUnderCursor), Model).IsSet();
		int interactionDistance = foundUnitLogic.Movement + foundUnitLogic.attackRange;

		if (unitCanTakeAction)
		{
			points = manhattanReachablePoints(
				Model.map.Width,
				Model.map.Height,
				interactionDistance,
				cursorPosition);
			points.Remove(cursorPosition);
		}

		auto filterer = [&](FInt16Range range)
		{
			return [&](FIntPoint point)
			{
				auto distanceVector = point - cursorPosition;
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

auto WorldStateReducer =
	[](FDungeonWorldState Model, TAction worldAction)
{
	return Visit(lager::visitor
	             {
		             [&](FBackAction)
		             {
			             return Visit(lager::visitor{
				                          [&](auto all)
				                          {
					                          auto Value = FBackTransitions{}(all);
					                          Model.InteractionContext.Set<decltype(Value)>(Value);
					                          return Model;
				                          },
			                          }, Model.InteractionContext);
		             },
		             [&](FChangeState action)
		             {
		             	Model.InteractionContext = action.newState;
		             	return Model;
		             },
		             [&](FEndTurnAction action)
		             {
			             const int MAX_PLAYERS = 2;
			             const auto nextTeamId = (Model.TurnState.teamId % MAX_PLAYERS) + 1;

			             Model.TurnState = {nextTeamId};
			             Model.InteractionContext.Set<FSelectingUnitContext>(FSelectingUnitContext());
			             return Model;
		             },
		             [&](FCursorPositionUpdated Updated)
		             {
			             Visit(FCursorPositionUpdatedHandler(Model, Updated.cursorPosition), Model.InteractionContext);
			             return Model;
		             },
		             [&](auto&& action)
		             {
			             using T = typename TDecay<decltype(action)>::Type;
			             if constexpr (TIsInTypeUnion<T, FCombatAction, FMoveAction, FWaitAction>::Value)
			             {
				             FDungeonLogicUnit& foundUnit = Model.map.loadedUnits.FindChecked(action.InitiatorId);
				             foundUnit.state = UnitState::ActionTaken;
				             Model.TurnState.unitsFinished.Add(foundUnit.Id);

				             lager::visitor{
					             [&](FMoveAction MoveAction)
					             {
						             FDungeonLogicMap& DungeonLogicMap = Model.map;
						             DungeonLogicMap.unitAssignment.Remove(
							             *DungeonLogicMap.unitAssignment.FindKey(MoveAction.InitiatorId));
						             DungeonLogicMap.unitAssignment.
						                             Add(MoveAction.Destination, MoveAction.InitiatorId);
					             },
					             [&](FCombatAction CombatAction)
					             {
						             FDungeonLogicUnit& Unit = CombatAction.updatedUnit;
						             Unit.state = Model.TurnState.unitsFinished.Contains(Unit.Id)
							                          ? UnitState::ActionTaken
							                          : UnitState::Free;
					             },
					             [&](FWaitAction WaitAction)
					             {
					             }
				             }(action);
			             }
		             	
			             return Model;
		             },
	             }, worldAction);
};

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

	// store = MakeUnique<lager::store<THistoryAction<TAction>, FHistoryModel<FDungeonWorldState>>>(
	//   lager::make_store<THistoryAction<TAction>>(
	//     FHistoryModel<FDungeonWorldState>(FDungeonWorldState(this->Game)),
	//     lager::with_manual_event_loop{},
	//     lager::with_reducer(WorldStateReducer),
	//     with_history
	//   ));

	store = MakeUnique<lager::store<TStoreAction, TModelType>>(
		lager::make_store<TStoreAction>(
			TModelType(this->Game),
			lager::with_manual_event_loop{},
			lager::with_reducer(WorldStateReducer)
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

	auto styleSetter = [&](FString&& fieldName, auto&& methodPointer) -> decltype(SVerticalBox::Slot())
	{
		return MoveTemp(SVerticalBox::Slot().AutoHeight()[
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
		]);
	};

	MainWidget->UnitDisplay->SetContent(
		SNew(SVerticalBox)
		+ styleSetter("Name", &ADungeonGameModeBase::GetLastSeenUnitUnderCursorName)
		+ styleSetter("HitPoints", &ADungeonGameModeBase::GetLastSeenUnitUnderCursorHitPoints)
		+ styleSetter("Movement", &ADungeonGameModeBase::GetLastSeenUnitUnderCursorMovement)
		+ styleSetter("Turn ID", &ADungeonGameModeBase::GetCurrentTurnId)
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
                     .Text_Lambda([&]()->auto
                     {
                     	return Visit([&](auto&& context)
                     	{
	                        return FText::FromString((TDecay<decltype(context)>::Type::StaticStruct()->GetName()));
                     	},store->get().InteractionContext);
                     })
          			]
          		]
	);
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

bool ADungeonGameModeBase::canUnitMoveToPointInRange(int unitId, FIntPoint destination,
                                                     const TSet<FIntPoint>& movementExtent)
{
	auto& map = Game.map;
	if (map.unitAssignment.Contains(destination))
		return false;

	if (map.unitAssignment.FindKey(unitId) == nullptr)
		return false;

	return movementExtent.Contains(destination) && Game.unitIdToActor.Contains(unitId);
}

TOptional<FDungeonLogicUnit> ADungeonGameModeBase::FindUnit(FIntPoint pt)
{
	const auto& DungeonLogicMap = lager::view(mapLens, *this);
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

void ADungeonGameModeBase::Dispatch(TAction&& unionAction)
{
	store->dispatch(unionAction);
}

FText ADungeonGameModeBase::GetCurrentTurnId() const
{
	return FCoerceToFText::Value(lager::view(turnStateLens, *this).teamId);
}
