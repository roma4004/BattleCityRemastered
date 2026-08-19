#pragma once

#include "enums/ObstacleType.h"
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

//NOTE: a cell is a quarter of a classic brick block, not a whole one - four across and four down
//make one block, and that is what lets a wall crumble by quarters
struct MapData
{
	std::size_t cols{};
	std::size_t rows{};
	std::vector<ObstacleType> cells{};

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
