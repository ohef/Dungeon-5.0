// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "lager/store.hpp"
#include "Logic/DungeonGameState.h"

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
  return TIsInTypeUnion<T, FMainMenu, FUnitMenu, FUnitInteraction>::Value;
}

using TStoreAction = TAction;
using TModelType = FDungeonWorldState;
using TDungeonStore = lager::store<TStoreAction, TModelType>;
