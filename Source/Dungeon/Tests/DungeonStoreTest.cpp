// Copyright Epic Games, Inc. All Rights Reserved.

#include "Dungeon.h"
#include "DungeonGameModeBase.h"
#include "DungeonReducer.hpp"
#include "DungeonUnitActor.h"
#include "Algo/Accumulate.h"
#include "Algo/FindLast.h"
#include "Algo/MinElement.h"
#include "Algo/RemoveIf.h"
#include "Containers/UnrealString.h"
#include "lager/event_loop/manual.hpp"
#include "Logic/StateQueries.hpp"
#include "Misc/AutomationTest.h"
#include "Serialization/Csv/CsvParser.h"
#include "zug/run.hpp"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDungeonStoreTest, "Dungeon.Core.Main",
                                 EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

void GenerateMoves(FDungeonWorldState& state)
{
	using namespace lager::lenses;

	//Generate all possible moves?
	TArray<TDungeonAction> stuffToDo;

	state.TurnState.teamId = 1;
	auto aiTeamId = state.TurnState.teamId;
	TMap<int, TSet<int>> TeamIdToUnitIds;

	for (TTuple<int, FDungeonLogicUnit> LoadedUnit : state.Map.LoadedUnits)
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

	auto localView = [&](auto... lenses)
	{
		constexpr auto numberOfParams = sizeof...(lenses);
		if constexpr (numberOfParams > 1)
		{
			return lager::view(fan(lenses...), state);
		}
		else
		{
			return lager::view(lenses..., state);
		}
	};

	using TClosestDistanceAndUnitId = TTuple<int, int>;

	//What the fuck
	TArray<TArray<TClosestDistanceAndUnitId>> closestUnitMap = TArray<TArray<TClosestDistanceAndUnitId>>();
	auto rowInit = TArray<TClosestDistanceAndUnitId>();
	rowInit.Init(TClosestDistanceAndUnitId(), state.Map.Height);
	closestUnitMap.Init(rowInit, state.Map.Height);

	for (int i = 0; i < state.Map.Width; i++)
	{
		for (int j = 0; j < state.Map.Height; j++)
		{
			auto thisPt = FIntPoint{i, j};
			TClosestDistanceAndUnitId minTuple = {INT_MAX, -1};

			for (int unitId : enemyIds)
			{
				auto hey = TClosestDistanceAndUnitId{
					manhattanDistance(thisPt - localView(unitIdToPosition(unitId))), unitId
				};
				minTuple = [](auto&& a, auto&& b) { return TLess<>{}(get<1>(a), get<1>(b)); }(minTuple, hey)
					           ? hey
					           : minTuple;
			}

			closestUnitMap[i][j] = minTuple;
		}
	}

	for (int AIUnitId : aiUnitIds)
	{
		auto [aiPosition,aiUnitData] = lager::view(fan(unitIdToPosition(AIUnitId), unitDataLens(AIUnitId)), state);
		auto interactablePoints = manhattanReachablePoints(
			state.Map.Width - 1,
			state.Map.Height - 1,
			aiUnitData->attackRange + aiUnitData->Movement,
			aiPosition);

		using TActionsTuple = TTuple<FMoveAction, TOptional<FCombatAction>>;

		TArray<TActionsTuple> actions;

		for (FIntPoint movePoint : interactablePoints)
		{
			auto moveA = FMoveAction(AIUnitId, movePoint);
			auto optCombat = TOptional<FCombatAction>();
			const auto& [distance, unitId] = closestUnitMap[movePoint.X][movePoint.Y];

			if (distance <= aiUnitData->attackRange)
			{
				optCombat = FCombatAction(AIUnitId, localView(unitDataLens(unitId))->Id, AIUnitId);
			}
			actions.Add(TActionsTuple(MoveTemp(moveA), MoveTemp(optCombat)));
		}

		Algo::Sort(actions, [&](TActionsTuple& a, TActionsTuple& b)
		{
			const auto& aPoint = a.Key.Destination;
			const auto& bPoint = b.Key.Destination;

			return !TLess<bool>{}(a.Value.IsSet(), b.Value.IsSet()) || TLess<int>()(
				closestUnitMap[aPoint.X][aPoint.Y].Key, closestUnitMap[bPoint.X][bPoint.Y].Key);
			// return !TLess<bool>{}(a.Value.IsSet(), b.Value.IsSet());
		});

		int g = 0;

		// if (enemyUnitToFUCKUP.IsSet())
		// {
		// 	auto attackablePointsAtEnemy = manhattanReachablePoints(
		// 		state.Map.Width,
		// 		state.Map.Height,
		// 		aiUnitData->attackRange,
		// 		enemyUnitPositionToFUCKUP);
		// 	// auto attackPosition = attackablePointsAtEnemy.begin().ElementIt->Value;
		// 	// TDungeonAction MoveAction = CreateDungeonAction(FMoveAction(AIUnitId, attackPosition));
		// 	// TDungeonAction CombatAction = CreateDungeonAction(FCombatAction(AIUnitId, *enemyUnitToFUCKUP, AIUnitId));
		// }
	}
}

TArray<TTuple<int, int, int>> ChooseNextUnitToActOn(FDungeonWorldState& state)
{
	using namespace lager::lenses;

	//Generate all possible moves?
	TArray<TDungeonAction> stuffToDo;

	state.TurnState.teamId = 1;
	auto aiTeamId = state.TurnState.teamId;
	TMap<int, TSet<int>> TeamIdToUnitIds;

	for (TTuple<int, FDungeonLogicUnit> LoadedUnit : state.Map.LoadedUnits)
	{
		int UnitTeamId = LoadedUnit.Value.teamId;
		auto& TeamSet = TeamIdToUnitIds.FindOrAdd(UnitTeamId);
		TeamSet.Add(LoadedUnit.Key);
	}

	auto aiUnitIds = TeamIdToUnitIds.FindAndRemoveChecked(aiTeamId);

	TArray<TTuple<int, int, int>> pairs;

	// auto movementPoints = manhattanReachablePoints(
	// 	state.Map.Width,
	// 	state.Map.Height,
	// 	aiUnit.Movement,
	// 	aiUnitPosition);

	for (int aiUnitId : aiUnitIds)
	{
		auto aiUnitPosition = lager::view(unitIdToPosition(aiUnitId), state);

		for (auto [teamId,unitIds] : TeamIdToUnitIds)
		{
			for (int unitId : unitIds)
			{
				auto enemyUnitPosition = lager::view(unitIdToPosition(unitId), state);
				int32 distanceBetweenPoints = (enemyUnitPosition - aiUnitPosition).SizeSquared();
				pairs.Add(MakeTuple(aiUnitId, unitId, distanceBetweenPoints));
			}
		}
	}

	auto wew = [](auto& a, auto& b)
	{
		return get<2>(a) < get<2>(b);
	};

	pairs.Sort(wew);

	return pairs;
}

bool FDungeonStoreTest::RunTest(const FString& Parameters)
{
	auto model = FHistoryModel();
	auto& Game = model.CurrentState();

	//(unitData=(Id=66,damage=2,HitPoints=20,state=Free,Name="66",Movement=3,teamId=1,attackRange=1),UnrealActor=BlueprintGeneratedClass'"/Game/Unit/Unit4.Unit4_C"')
	//(unitData=(Id=67,damage=2,HitPoints=13,state=Free,Name="67",Movement=8,teamId=2,attackRange=1),UnrealActor=BlueprintGeneratedClass'"/Game/Unit/Unit3.Unit3_C"')

	// TArray<FDungeonLogicUnitRow*> unitsArray;
	// UnitTable->GetAllRows(TEXT(""), unitsArray);
	TMap<int, FDungeonLogicUnitRow> loadedUnitTypes;
	loadedUnitTypes.Add(66, FDungeonLogicUnitRow(FDungeonLogicUnit(66, 2, 20, Free, "66", 3, 1, 1)));
	loadedUnitTypes.Add(67, FDungeonLogicUnitRow(FDungeonLogicUnit(67, 2, 13, Free, "67", 8, 2, 1)));
	// for (auto unit : unitsArray)
	// {
	// 	loadedUnitTypes.Add(unit->unitData.Id, *unit);
	// }

	FString StuffR;
	FString TheStr = FPaths::Combine(FPaths::ProjectDir(), TEXT("theBoard.csv"));
	if (FFileHelper::LoadFileToString(StuffR, ToCStr(TheStr)))
	{
		const FCsvParser Parser(MoveTemp(StuffR));
		const auto& Rows = Parser.GetRows();
		int32 height = Rows.Num();

		Game.Map.LoadedTiles.Add(1, {1, "Grass", 1});

		for (int i = 0; i < height; i++)
		{
			int32 width = Rows[i].Num();
			Game.Map.Height = height;
			Game.Map.Width = width;
			Game.TurnState.teamId = 1;
			for (int j = 0; j < width; j++)
			{
				int dataIndex = i * width + j;
				auto value = FCString::Atoi(Rows[i][j]);

				Game.Map.TileAssignment.Add({i, j}, 1);

				if (loadedUnitTypes.Contains(value))
				{
					auto zaRow = loadedUnitTypes[value];
					auto zaunit = zaRow.unitData;
					zaunit.Id = dataIndex;
					zaunit.Name = FString::FormatAsNumber(dataIndex);
					Game.Map.LoadedUnits.Add(zaunit.Id, zaunit);

					FIntPoint positionPlacement{i, j};
					Game.Map.UnitAssignment.Add(positionPlacement, dataIndex);
				}
			}
		}
	}

	auto store = MakeUnique<TDungeonStore>(TDungeonStore(lager::make_store<TStoreAction>(
		model,
		lager::with_manual_event_loop{},
		WithUndoReducer(WorldStateReducer)
	)));

	// constructors must create equal objects
	{
		store->dispatch(TDungeonAction(TInPlaceType<FCursorPositionUpdated>{}, FIntPoint{1, 4}));
		store->dispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, FIntPoint{1, 4}));

		int ViewState = lager::view(GetUnitIdFromContext, store->get());
		store->dispatch(
			TDungeonAction(
				TInPlaceType<FSteppedAction>{},
				TStepAction(TInPlaceType<FMoveAction>{}, ViewState),
				TArray{
					TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{})
				}));

		TestTrue("Should be in SelectingUnitAbilityTarget",
		         store->zoom(interactionContextLens).make().get().IsType<FSelectingUnitAbilityTarget>());

		store->dispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, FIntPoint{1, 6}));

		TestTrue("Should be in UnitMenu", store->zoom(interactionContextLens).make().get().IsType<FUnitMenu>());

		store->dispatch(TDungeonAction(TInPlaceType<FBackAction>{}));
		store->dispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, FIntPoint{1, 6}));
		store->dispatch(TDungeonAction(TInPlaceType<FBackAction>{}));

		TestTrue("Should be in SelectingUnitAbilityTarget",
		         store->zoom(interactionContextLens).make().get().IsType<FSelectingUnitAbilityTarget>());
	}

	{
		FDungeonWorldState modell;
		modell.CursorPosition = {1, 2};
		modell.Map.Width = 10;
		modell.Map.Height = 10;
		modell.Map.UnitAssignment.Add({1, 2}, 1);
		modell.Map.UnitAssignment.Add({0, 2}, 2);
		modell.Map.UnitAssignment.Add({2, 2}, 3);

		auto attackingUnit = FDungeonLogicUnit(1, 2, 13, Free, "67", 8, 2, 1);
		auto targetedUnit = FDungeonLogicUnit(2, 2, 13, Free, "67", 8, 1, 1);

		modell.Map.LoadedUnits.Add(1, attackingUnit);
		modell.Map.LoadedUnits.Add(2, targetedUnit);
		modell.Map.LoadedUnits.Add(3, FDungeonLogicUnit(3, 2, 13, Free, "67", 8, 1, 1));

		auto result = GetInteractablePositionsAtCursorPosition(modell);

		TestTrue("Should be 2", result.Num() == 2);

		TArray<FIntPoint> IntPoints = zug::unreal::into(TArray<FIntPoint>{}, zug::identity, result);
		auto cycler = TCycleArrayIterator(IntPoints);

		TArray<FIntPoint>::ElementType IntPoint = cycler.Current().GetValue();
		TestTrue("Starts at 0", IntPoint == IntPoints[0]);
		TestTrue("Should be 2", cycler.Forward().GetValue() == IntPoints[1]);
		TestTrue("Should be 2", cycler.Forward().GetValue() == IntPoints[0]);
	}

	{
		FDungeonWorldState modell;
		modell.CursorPosition = {1, 2};
		modell.Map.Width = 10;
		modell.Map.Height = 10;
		modell.Map.UnitAssignment.Add({1, 2}, 1);
		modell.Map.UnitAssignment.Add({0, 2}, 2);
		modell.Map.UnitAssignment.Add({8, 8}, 3);

		auto attackingUnit = FDungeonLogicUnit(1, 2, 13, Free, "67", 8, 2, 1);
		auto targetedUnit = FDungeonLogicUnit(2, 2, 13, Free, "67", 8, 1, 1);

		modell.Map.LoadedUnits.Add(1, attackingUnit);
		modell.Map.LoadedUnits.Add(2, targetedUnit);
		modell.Map.LoadedUnits.Add(3, FDungeonLogicUnit(3, 2, 13, Free, "67", 8, 1, 1));

		GenerateMoves(modell);
		// auto distancePrioritizedUnits = ChooseNextUnitToActOn(modell);
		// auto DistancePrioritizedUnit = distancePrioritizedUnits.Pop();
	}
	
	{
		FDungeonWorldState modell;
		modell.CursorPosition = {1, 2};
		modell.Map.Width = 10;
		modell.Map.Height = 10;
		modell.Map.UnitAssignment.Add({1, 2}, 1);
		modell.Map.UnitAssignment.Add({0, 2}, 2);
		modell.Map.UnitAssignment.Add({8, 8}, 3);

		auto attackingUnit = FDungeonLogicUnit(1, 2, 13, Free, "67", 8, 2, 1);
		auto targetedUnit = FDungeonLogicUnit(2, 2, 13, Free, "67", 8, 1, 1);

		// WithUndoReducer(WorldStateReducer);

		modell.Map.LoadedUnits.Add(1, attackingUnit);
		modell.Map.LoadedUnits.Add(2, targetedUnit);
		modell.Map.LoadedUnits.Add(3, FDungeonLogicUnit(3, 2, 13, Free, "67", 8, 1, 1));
	}

	return true;
}

#endif //WITH_DEV_AUTOMATION_TESTS
