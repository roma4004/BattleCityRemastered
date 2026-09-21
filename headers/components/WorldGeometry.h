#pragma once

#include "geometry/Point.h"
#include <cmath>
#include <cstddef>

struct WorldGeometry
{
	//NOTE: a cell is a quarter of a classic brick block, which is what lets a wall crumble by quarters
	static constexpr double kCellSize{12.0};
	//NOTE: a tank is three quarters of a brick block - narrower than the corridors it drives, and the gap
	//a map has to leave at its edges for anything to spawn
	static constexpr std::size_t kTankCellSpan{3u};
	//NOTE: how thick the map draws the wall around the eagle - the spawner cuts the seats around it,
	//so the number has to match the .map file
	static constexpr std::size_t kFortressRingCells{2u};
	//NOTE: 176, not the sprite's 175 - 624 + 176 is a round 800x600 at 4:3
	static constexpr std::size_t kSideBarWidth{176u};
	static constexpr UPoint kClassicBattlefieldSize{.x = 52u * 12u, .y = 50u * 12u};

	//NOTE: the least a shot has to blow out so a tank fits through the hole - the far corner of the first
	//row lies half a tank across and half a bullet ahead, and a flush touch is not a hit
	[[nodiscard]] static double BlastRadiusFor(const double tankSize, const double bulletLength) noexcept
	{
		constexpr double margin{1.0};

		return std::hypot(tankSize / 2.0, bulletLength / 2.0) + margin;
	}

	[[nodiscard]] static UPoint ForMap(std::size_t cols, std::size_t rows);
};
