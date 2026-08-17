#include "components/WorldGeometry.h"
#include <algorithm>

WorldGeometry WorldGeometry::FitMap(const UPoint windowSize, const std::size_t cols, const std::size_t rows)
{
	if (cols == 0u || rows == 0u)
	{
		return {};
	}

	//NOTE: squeezing the bar to its minimum is what a wide map is allowed to do before the cell shrinks
	const auto widestField = static_cast<float>(windowSize.x - std::min<std::size_t>(windowSize.x, kMinSideBarWidth));
	const float byWidth = widestField / static_cast<float>(cols);
	const float byHeight = static_cast<float>(windowSize.y) / static_cast<float>(rows);

	//NOTE: one cell size for both axes - a rectangular cell would make the tank rectangular too, and
	//the bullet's circular blast radius elliptic
	const float cellSize = std::min(byWidth, byHeight);

	const UPoint battlefieldSize{.x = static_cast<unsigned>(cellSize * static_cast<float>(cols)),
								 .y = static_cast<unsigned>(cellSize * static_cast<float>(rows))};

	return {.cellSize = cellSize,
			.battlefieldSize = battlefieldSize,
			.sideBarWidth = windowSize.x - battlefieldSize.x};
}
