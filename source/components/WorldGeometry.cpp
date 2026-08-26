#include "components/WorldGeometry.h"

UPoint WorldGeometry::ForMap(const std::size_t cols, const std::size_t rows)
{
	if (cols == 0u || rows == 0u)
	{
		return {};
	}

	//NOTE: one cell size for both axes - a rectangular cell would make the tank rectangular too, and
	//the bullet's circular blast radius elliptic
	return UPoint{.x = static_cast<std::size_t>(static_cast<float>(cols) * kCellSize),
				  .y = static_cast<std::size_t>(static_cast<float>(rows) * kCellSize)};
}
