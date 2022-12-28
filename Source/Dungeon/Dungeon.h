// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "lager/store.hpp"
#include "Logic/DungeonGameState.h"

// DEFINE_LOG_CATEGORY(DungeonLog);

struct FDungeonWorldState;
struct FUnitInteraction;
struct FUnitMenu;
struct FMainMenu;

DECLARE_EVENT_OneParam(AMapCursorPawn, FQueryInput, FIntPoint);

template <typename T, typename ...TArgs>
struct TIsInTypeUnion
{
	enum { Value = TOr<TIsSame<T, TArgs>...>::Value };
};

template <typename T>
constexpr bool isInGuiControlledState()
{
	return TIsInTypeUnion<typename TDecay<T>::Type, FMainMenu, FUnitMenu, FUnitInteraction>::Value;
}

template <class... Ts>
using TDungeonVisitor = lager::visitor<Ts...>;

using TStoreAction = TDungeonAction;
using TModelType = FDungeonWorldState;

struct FHistoryModel
{
	TModelType committedState;
	TArray<TModelType> undoStack;

	FHistoryModel() : committedState(TModelType()){
	}
	
	FHistoryModel(const TModelType& m) : committedState(m){
	}

	bool CanGoBack()
	{
		return !undoStack.IsEmpty();
	}

	auto& Commit(){
		if (!undoStack.IsEmpty())
		{
			undoStack.Empty();
		}
		return *this;
	}

	auto& CurrentState() 
	{
		return committedState;
	}

	auto& AddRecord(TModelType t)
	{
		undoStack.Push(t);
		return *this;
	}

	operator TModelType() const
	{
		return committedState;
	}

	const auto& GoBack()
	{
		if (undoStack.IsEmpty())
			return *this;

		committedState = undoStack.Pop();

		return *this;
	}
};

using TDungeonStore = lager::store<TStoreAction, FHistoryModel>;
