#pragma once

#include "enums/BonusType.h"
#include "enums/ObstacleType.h"
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

//NOTE: a bonus the map lays out itself - the cell marks its top-left corner, the way the eagle's does
struct BonusPlacement
{
	std::size_t col{};
	std::size_t row{};
	BonusType type{};
};

struct MapData
{
	std::size_t cols{};
	std::size_t rows{};
	std::vector<ObstacleType> cells{};
	std::vector<BonusPlacement> bonuses{};

	[[nodiscard]] ObstacleType At(const std::size_t col, const std::size_t row) const
	{
		return cells[row * cols + col];
	}
};

struct MapError
{
	std::filesystem::path path{};
	std::string reason{};
	//NOTE: 1-based, 0 when the whole file is at fault rather than one line of it
	std::size_t line{};
};
