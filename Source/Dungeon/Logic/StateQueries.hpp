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

const auto GetInteractablePositionss = [](const FDungeonWorldState& Model, TFunctionRef<FIntPoint()> positionGetter)
{
	const auto selector = [&Model](auto&& lens) { return lager::view(lens, Model); };

	auto cursorPosition = positionGetter();

	auto unitId = selector(getUnitAtPointLens(cursorPosition));
	if (!unitId.IsSet())
		return TSet<FIntPoint>();

	const auto& unitData = selector(unitDataLens(*unitId) | ignoreOptional);

	auto points = manhattanReachablePoints(
		Model.Map.Width,
		Model.Map.Height,
		unitData.attackRange,
		cursorPosition);

	const auto xf = zug::filter([&](FIntPoint Point)
	{
		auto possibleUnit = selector(getUnitAtPointLens(Point));
		return possibleUnit.IsSet() && selector(unitDataLens(*possibleUnit))->teamId != unitData.teamId;
	});

	return zug::unreal::into(TSet<FIntPoint>(), xf, points);
};

const auto GetInteractablePositionsUsingTarget = [](const FDungeonWorldState& Model, const FIntPoint& pt)
{
	return GetInteractablePositionss(Model, [&] { return pt; });
};

const auto GetInteractablePositions = [](const FDungeonWorldState& Model)
{
	const auto selector = [&Model](auto&& lens) { return lager::view(lens, Model); };
	return GetInteractablePositionss(Model, [&] { return selector(attr(&FDungeonWorldState::CursorPosition)); });
};
