#pragma once

#include "components/WorldGeometry.h"

enum class ObstacleType : char8_t
{
	None,

	Brick,
	Steel,
	Eagle,
	Fortress,
	Water,
	Bush,
	Ice,

	lastId
};

//NOTE: the gate for values from a map file or the wire
[[nodiscard]] constexpr bool IsSpawnableObstacle(const ObstacleType type) noexcept
{
	return type > ObstacleType::None && type < ObstacleType::lastId;
}

//NOTE: ground a tank drives over - spawn spots and map rules count it open; in step with Passable of BushTile, IceTile
[[nodiscard]] constexpr bool IsDrivableObstacle(const ObstacleType type) noexcept
{
	return type == ObstacleType::Bush || type == ObstacleType::Ice;
}

//NOTE: the eagle is one map cell but tank-sized; here because the map sizes it on the host, the spawn event on a client
[[nodiscard]] constexpr double ObstacleCellSpan(const ObstacleType type) noexcept
{
	return type == ObstacleType::Eagle ? static_cast<double>(WorldGeometry::kTankCellSpan) : 1.0;
}
