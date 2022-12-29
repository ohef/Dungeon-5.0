#pragma once

#include "SimpleCastTo.hpp"
#include "Dungeon/Utility/LagerIntegration.hpp"
#include "lager/lenses/attr.hpp"
#include "lager/lenses.hpp"

template <class T, class M>
M get_member_type(M T::*);
#define GET_TYPE_OF(mem) decltype(get_member_type(mem))

using namespace lager::lenses;

const auto gameLens = attr(&ADungeonGameModeBase::store)
  | deref
  | lager::lenses::getset([](auto&& x)
                          {
                            return LAGER_FWD(x.get());
                          }, [](auto&& x)
                          {
                            return x;
                          })
  | SimpleCastTo<FDungeonWorldState>;

const auto mapLens = gameLens
  | attr(&FDungeonWorldState::map);

const auto turnStateLens = gameLens
  | attr(&FDungeonWorldState::TurnState);
  
const auto isUnitFinishedLens = [&](int id)
{
  return turnStateLens
    | attr(&FTurnState::unitsFinished)
    | Find(id);
};

const auto isUnitFinishedLens2 = [&](int id)
{
  return attr(&FDungeonWorldState::TurnState)
    | attr(&FTurnState::unitsFinished)
    | Find(id);
};

const auto gameModeLens =
  getter(&UWorld::GetAuthGameMode<ADungeonGameModeBase>)
  | deref;

const auto worldStoreLens = gameModeLens
  | lager::lenses::attr(&ADungeonGameModeBase::store);

const auto interactionContextLens =
  SimpleCastTo<FDungeonWorldState>
  | lager::lenses::attr(&FDungeonWorldState::InteractionContext);

const auto isMainMenuLens =
  interactionContextLens
  | unreal_alternative<FMainMenu>;
                        
const auto getUnitAtPointLens = [](const FIntPoint& pt)
{
  return attr(&FDungeonWorldState::map)
    | attr(&FDungeonLogicMap::unitAssignment)
    | Find(pt);
};

const auto cursorPositionLens = attr(&FDungeonWorldState::CursorPosition);

const auto unitIdToPosition = [](int unitId)
{
  using unitAssign_t = GET_TYPE_OF(&FDungeonLogicMap::unitAssignment);
  using ReturnType = unitAssign_t::KeyType;

  return SimpleCastTo<FDungeonWorldState>
    | attr(&FDungeonWorldState::map)
    | attr(&FDungeonLogicMap::unitAssignment)
    | lager::lenses::getset(
      [unitId](unitAssign_t&& map)
      {
        return *map.FindKey(unitId);
      },
      [](unitAssign_t&& map, unitAssign_t::KeyType&& positionToSet)
      {
        return ReturnType();
      });
};

const auto unitIdToActor = [](int unitId)
{
  return SimpleCastTo<FDungeonWorldState>
             | attr(&FDungeonWorldState::unitIdToActor) 
             | Find(unitId);
};

const auto unitDataLens = [](int id)
{
  return attr(&FDungeonWorldState::map)
    | attr(&FDungeonLogicMap::loadedUnits)
    | Find(id);
};

const auto unitIdToData = [](int id)
{
  return unitDataLens(id) | ignoreOptional;
};

const auto getUnitUnderCursor =
  lager::lenses::getset([&](const FDungeonWorldState& m) -> TOptional<FDungeonLogicUnit> 
                        {
                          TOptional<int> v = lager::view(getUnitAtPointLens(lager::view(cursorPositionLens, m)), m);
                          if(!v.IsSet())
                            return {};

                          return lager::view(unitDataLens(*v), m);
                        }, [](auto m, auto) { return m; });
