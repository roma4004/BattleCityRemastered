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

//NOTE: the one gate for values that arrive from outside the code - a map file, or the wire. None is
//not spawnable either: it is the absence of an obstacle, not a kind of one.
[[nodiscard]] constexpr bool IsSpawnableObstacle(const ObstacleType type) noexcept
{
	return type > ObstacleType::None && type < ObstacleType::lastId;
}

//NOTE: the eagle takes one map cell but covers a whole tank, the way the fortress ring is laid out in the file.
//The map sizes it on the host and the spawn event on the client, so the rule lives here
[[nodiscard]] constexpr double ObstacleCellSpan(const ObstacleType type) noexcept
{
	return type == ObstacleType::Eagle ? static_cast<double>(WorldGeometry::kTankCellSpan) : 1.0;
}
