#include "components/WorldGeometry.h"

UPoint WorldGeometry::ForMap(const std::size_t cols, const std::size_t rows)
{
	if (cols == 0u || rows == 0u)
	{
		return {};
	}

	//NOTE: one cell size for both axes keeps the tank square and the bullet's blast circular
	return UPoint{.x = static_cast<std::size_t>(static_cast<double>(cols) * kCellSize),
				  .y = static_cast<std::size_t>(static_cast<double>(rows) * kCellSize)};
}
