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

inline auto GetInteractablePositions(const FDungeonWorldState& Model, TFunctionRef<FIntPoint()> positionGetter)
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
}

inline decltype(auto) GetInteractablePositions(const FDungeonWorldState& Model, const FIntPoint& pt)
{
	return GetInteractablePositions(Model, [&] { return pt; });
};

inline decltype(auto) GetInteractablePositionsAtCursorPosition(const FDungeonWorldState& Model)
{
	const auto selector = [&Model](auto&& lens) { return lager::view(lens, Model); };
	return GetInteractablePositions(Model, [&] { return selector(attr(&FDungeonWorldState::CursorPosition)); });
};
