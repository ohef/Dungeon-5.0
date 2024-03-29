﻿#pragma once
#include "lager/lenses.hpp"
#include "Macros.hpp"
#include "Lenses/SimpleCastTo.hpp"

class ADungeonGameModeBase;

template<typename TThis, class TVariantAction>
struct FStoreConnectedClass
{
  virtual ~FStoreConnectedClass(){};

  template<typename T>
  void StoreDispatch(T&& action)
  {
    static_cast<TThis*>(this)
      ->GetWorld()
      ->template GetAuthGameMode<ADungeonGameModeBase>()
      ->Dispatch(Forward<T>(action));
  }
  
  void StoreDispatch(TVariantAction&& action)
  {
    static_cast<TThis*>(this)
    ->GetWorld()
    ->template GetAuthGameMode<ADungeonGameModeBase>()
    ->Dispatch(Forward<decltype(action)>(action));
  }

  decltype(auto) UseState(auto&& lens) {
    return static_cast<TThis*>(this)
    ->GetWorld()
    ->template GetAuthGameMode<ADungeonGameModeBase>()
    ->store
    ->zoom(SimpleCastTo<FDungeonWorldState> | lens);
  }
  
  decltype(auto) UseStoreNode() {
    return static_cast<TThis*>(this)
    ->GetWorld()
    ->template GetAuthGameMode<ADungeonGameModeBase>()
    ->store
    ->zoom(SimpleCastTo<FDungeonWorldState>);
  }
  
  decltype(auto) UseViewState() {
    return UseViewState(SimpleCastTo<FDungeonWorldState>);
  }
  
  decltype(auto) UseViewState(auto&& lens) {
    return lager::view(SimpleCastTo<FDungeonWorldState> | lens, static_cast<TThis*>(this)
    ->GetWorld()
    ->template GetAuthGameMode<ADungeonGameModeBase>()
    ->store
    ->get());
  }
  
  auto& UseEvent() {
    return static_cast<TThis*>(this)
    ->GetWorld()
    ->template GetAuthGameMode<ADungeonGameModeBase>()
    ->DungeonActionDispatched;
  }
};
