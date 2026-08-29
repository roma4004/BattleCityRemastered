#pragma once

#include "geometry/Point.h"
#include <cstddef>

struct WorldGeometry
{
	//NOTE: a cell is a quarter of a classic brick block, which is what lets a wall crumble by quarters
	static constexpr double kCellSize{12.0};
	static constexpr std::size_t kSideBarWidth{175u};
	static constexpr UPoint kClassicBattlefieldSize{.x = 52u * 12u, .y = 50u * 12u};

	[[nodiscard]] static UPoint ForMap(std::size_t cols, std::size_t rows);
};
