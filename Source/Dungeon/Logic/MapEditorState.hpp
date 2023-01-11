#pragma once
#include "map.h"
#include "unit.h"
#include "Components/Button.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Utility/ZugIntegration.hpp"

struct FMapEditorState
{
	TArray<FDungeonLogicUnit> UnitPrefabs;
	FDungeonLogicMap CurrentMap;
};

// using TValue = TVariant<FEmptyVariantState, int, FString>;
using TValue = FString;

struct FNode;

struct FNode
{
	FName ComponentName;
	TMap<FName, TValue> Props;
	TSet<TSharedPtr<FNode>> Children;
};

inline FNode RenderMapEditorState(FMapEditorState&& State)
{
	auto mapper =
	[](TTuple<FIntPoint, int>& UnitAssignment)
    	{
    		return FNode{
    			.ComponentName = "Slot",
    			.Props = {
    				{"Column", FString::FormatAsNumber(UnitAssignment.Key.X)},
    				{"Row", FString::FormatAsNumber(UnitAssignment.Key.Y)}
    			}
    		};
    	};

	TSet<TSharedPtr<FNode>> nodes;
	for (TTuple<FIntPoint, int> tuple : State.CurrentMap.UnitAssignment)
	{
		nodes.Add(MakeShared<FNode>(mapper(tuple)));
	}

	return FNode{.ComponentName = "Root", .Children = nodes};
}

template <typename TValueType = TSharedPtr<FNode>>
struct FCaseSensitiveLookupKeyFuncs : BaseKeyFuncs<TValueType, FString>
{
	static FORCEINLINE FString GetSetKey(const TValueType& Element)
	{
		return *Element->Props.Find("Column") + *Element->Props.Find("Row");
	}

	static FORCEINLINE bool Matches(const FString& A, const FString& B)
	{
		return A.Equals(B, ESearchCase::CaseSensitive);
	}

	static FORCEINLINE uint32 GetKeyHash(const FString& Key)
	{
		return FCrc::StrCrc32<TCHAR>(*Key);
	}
};

inline void Reconcile(const FNode& previousTree, const FNode& nextTree, UGridPanel& SomeShit )
{
	TSet<TSharedPtr<FNode>, FCaseSensitiveLookupKeyFuncs<>> added;
	TSet<TSharedPtr<FNode>, FCaseSensitiveLookupKeyFuncs<>> removed;
	TSet<TSharedPtr<FNode>, FCaseSensitiveLookupKeyFuncs<>> nextTreeForDiff;
	TSet<TSharedPtr<FNode>, FCaseSensitiveLookupKeyFuncs<>> previousTreeForDiff;

	for (auto Child : previousTree.Children)
		previousTreeForDiff.Add(Child);
	for (auto Child : nextTree.Children)
		nextTreeForDiff.Add(Child);

	TSharedPtr<FNode> testThing = MakeShared<FNode>(FNode{ .ComponentName = "wew", .Props = {{"Column", "1"}, {"Row", "2"}} });

	auto testa = *testThing->Props.Find("Column"); 
	auto testb = *testThing->Props.Find("Row"); 
	
	auto hey = FCaseSensitiveLookupKeyFuncs<>::GetSetKey(testThing);
	auto result = nextTreeForDiff.Contains(hey);

	auto diff = previousTreeForDiff.Difference(nextTreeForDiff) ;
	
	for (const auto& nextTreeNode : diff)
	{
		if (!previousTree.Children.Contains(nextTreeNode))
		{
			added.Add(nextTreeNode);
		}
		
		if(!nextTree.Children.Contains(nextTreeNode)){
			removed.Add(nextTreeNode);
		}
	}

	auto copy = SomeShit.GetSlots();
	
	for (auto Shit : copy)
	{
		auto GridSlot = Cast<UGridSlot>(Shit);
		auto GridKey = FString::FormatAsNumber(GridSlot->Row) + FString::FormatAsNumber(GridSlot->Column);
		
		if (added.Contains(GridKey))
		{
			auto button = NewObject<UButton>();
			auto node = *added.Find(GridKey);

			SomeShit.AddChildToGrid(
				GridSlot->Content,
				FCString::Atoi(**node->Props.Find("Row")),
				FCString::Atoi(**node->Props.Find("Column"))
			);
		}
		
		if (removed.Contains(GridKey))
		{
			SomeShit.RemoveChild(GridSlot->Content);
		}
	}
}