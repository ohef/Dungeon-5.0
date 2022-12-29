#pragma once
#include "DungeonGameState.h"
#include "util.h"
#include "lager/lenses/optional.hpp"
#include "Lenses/model.hpp"
#include "Utility/ZugIntegration.hpp"

const auto GetUnitMenuContext = interactionContextLens
	| unreal_alternative_pipeline<FUnitMenu>;

const auto GetUnitIdFromContext = GetUnitMenuContext 
	| map_opt(attr(&FUnitMenu::unitId))
	| value_or(-1);

const auto GetInteractablePositions = [](const FDungeonWorldState& Model)
{
	const auto selector = [&Model](auto&& lens) { return lager::view(lens, Model); };

	auto cursorPosition = selector(attr(&FDungeonWorldState::CursorPosition));
	
	auto unitId = selector(getUnitAtPointLens(cursorPosition));
	if(!unitId.IsSet())
		return TSet<FIntPoint>();
		
	const auto& unitData = selector(unitDataLens(*unitId) | ignoreOptional);

	auto points = manhattanReachablePoints(
		Model.map.Width,
		Model.map.Height,
		unitData.attackRange,
		cursorPosition);

	const auto xf = zug::filter([&](FIntPoint Point)
	{
		auto possibleUnit = selector(getUnitAtPointLens(Point));
		return possibleUnit.IsSet() && selector(unitDataLens(*possibleUnit))->teamId != unitData.teamId;
	});

	return zug::unreal::into(TSet<FIntPoint>(), xf, points);
};