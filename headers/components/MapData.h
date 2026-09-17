#pragma once

#include "enums/ObstacleType.h"
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

//NOTE: four cells across and four down make one classic brick block, so a wall crumbles by quarters
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
