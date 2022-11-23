#pragma once
#include "lager/lenses.hpp"
#include "Macros.hpp"

class ADungeonGameModeBase;

template<typename TThis, class TVariantAction>
struct FStoreConnectedClass
{
  virtual ~FStoreConnectedClass(){};
  
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
    ->zoom(DUNGEON_FOWARD(lens));
  }
  
  decltype(auto) UseViewState(auto&& lens) {
    return lager::view(DUNGEON_FOWARD(lens), static_cast<TThis*>(this)
    ->GetWorld()
    ->template GetAuthGameMode<ADungeonGameModeBase>()
    ->store
    ->get());
  }
};
