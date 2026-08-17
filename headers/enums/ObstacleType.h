#pragma once

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
[[nodiscard]] constexpr bool IsSpawnableObstacle(const ObstacleType type)
{
	return type > ObstacleType::None && type < ObstacleType::lastId;
}
