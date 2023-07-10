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

bool FDungeonStoreTest::RunTest(const FString& Parameters)
{
	auto model = FHistoryModel();
	auto& Game = model.CurrentState();

	//(unitData=(Id=66,damage=2,HitPoints=20,state=Free,Name="66",Movement=3,teamId=1,attackRange=1),UnrealActor=BlueprintGeneratedClass'"/Game/Unit/Unit4.Unit4_C"')
	//(unitData=(Id=67,damage=2,HitPoints=13,state=Free,Name="67",Movement=8,teamId=2,attackRange=1),UnrealActor=BlueprintGeneratedClass'"/Game/Unit/Unit3.Unit3_C"')

	TMap<int, FDungeonLogicUnitRow> loadedUnitTypes;
	loadedUnitTypes.Add(66, FDungeonLogicUnitRow(FDungeonLogicUnit(66, 2, 20, Free, "66", 3, 1, 1)));
	loadedUnitTypes.Add(67, FDungeonLogicUnitRow(FDungeonLogicUnit(67, 2, 13, Free, "67", 8, 2, 1)));

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

	// auto store = MakeUnique<TDungeonStore>(TDungeonStore(lager::make_store<TStoreAction>(
	// 	model,
	// 	lager::with_manual_event_loop{},
	// 	WithUndoReducer(WorldStateReducer)
	// )));
	//
	// // constructors must create equal objects
	// {
	// 	store->dispatch(TDungeonAction(TInPlaceType<FCursorPositionUpdated>{}, FIntPoint{1, 4}));
	// 	store->dispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, FIntPoint{1, 4}));
	//
	// 	int ViewState = lager::view(getUnitUnderCursor, store->get())->Id;
	// 	store->dispatch(
	// 		TDungeonAction(
	// 			TInPlaceType<FSteppedAction>{},
	// 			TStepAction(TInPlaceType<FMoveAction>{}, ViewState),
	// 			TArray{
	// 				TInteractionContext(TInPlaceType<FSelectingUnitAbilityTarget>{})
	// 			}));
	//
	// 	TestTrue("Should be in SelectingUnitAbilityTarget",
	// 	         store->zoom(interactionContextLens).make().get().IsType<FSelectingUnitAbilityTarget>());
	//
	// 	store->dispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, FIntPoint{1, 6}));
	//
	// 	TestTrue("Should be in UnitMenu", store->zoom(interactionContextLens).make().get().IsType<FUnitMenu>());
	//
	// 	store->dispatch(TDungeonAction(TInPlaceType<FBackAction>{}));
	// 	store->dispatch(TDungeonAction(TInPlaceType<FCursorQueryTarget>{}, FIntPoint{1, 6}));
	// 	store->dispatch(TDungeonAction(TInPlaceType<FBackAction>{}));
	//
	// 	TestTrue("Should be in SelectingUnitAbilityTarget",
	// 	         store->zoom(interactionContextLens).make().get().IsType<FSelectingUnitAbilityTarget>());
	// }

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
