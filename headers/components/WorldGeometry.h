#pragma once

#include "geometry/Point.h"
#include <algorithm>
#include <cmath>
#include <cstddef>

struct WorldGeometry
{
	//NOTE: a cell is a quarter of a classic brick block, which is what lets a wall crumble by quarters
	static constexpr double kCellSize{12.0};
	static constexpr std::size_t kTankCellSpan{3u};
	static constexpr std::size_t kMaxTankCellSpan{4u};
	static constexpr std::size_t kFortressRingCells{2u};
	//NOTE: 176, not the sprite's 175 - 624 + 176 is a round 800x600 at 4:3
	static constexpr std::size_t kSideBarWidth{176u};
	static constexpr UPoint kClassicBattlefieldSize{.x = 52u * 12u, .y = 50u * 12u};

	//NOTE: IsCollide needs a strict overlap - a blast that only reaches a brick's edge would leave it standing
	static constexpr double kBlastMargin{1.0};

	//NOTE: least radius to break a tank-wide row - its outer corner lies half a tank aside and half a shell ahead
	[[nodiscard]] static double BlastRadiusFor(const double tankSize, const double bulletLength) noexcept
	{
		return std::hypot(tankSize / 2.0, bulletLength / 2.0) + kBlastMargin;
	}

	//NOTE: how deep past a wall's face a shell may deal damage and still break the face tank-wide; 0 if it never can
	[[nodiscard]] static double BlastDepthFor(const double tankSize, const double blastRadius) noexcept
	{
		const double reach{blastRadius - kBlastMargin};
		const double halfTank{tankSize / 2.0};

		return std::sqrt(std::max(reach * reach - halfTank * halfTank, 0.0));
	}

	[[nodiscard]] static UPoint ForMap(std::size_t cols, std::size_t rows);
};
