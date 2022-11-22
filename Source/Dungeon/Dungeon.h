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
  return TIsInTypeUnion<T, FMainMenu, FUnitMenu, FUnitInteraction>::Value;
}

template <class... Ts>
using TDungeonVisitor = lager::visitor<Ts...>;

using TStoreAction = TDungeonAction;
using TModelType = FDungeonWorldState;
using TDungeonStore = lager::store<TStoreAction, TModelType>;
