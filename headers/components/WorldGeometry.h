#pragma once

#include "Point.h"
#include <cstddef>

//NOTE: the single place that turns "a map of cols x rows cells" into pixels. The window is fixed -
//it comes from config.ini and from the player's own resize - so what gives is, in order: the side
//bar's width, and then the cell size. A map that still does not fit simply leaves the bottom empty.
struct WorldGeometry
{
	float cellSize{};
	UPoint battlefieldSize{};
	std::size_t sideBarWidth{};

	//NOTE: what the bar would like to be; it keeps whatever width the battlefield does not need
	static constexpr std::size_t kPreferredSideBarWidth{175u};
	//NOTE: below this the icons and the score stop fitting, so the cell shrinks instead
	static constexpr std::size_t kMinSideBarWidth{120u};

	[[nodiscard]] static WorldGeometry FitMap(UPoint windowSize, std::size_t cols, std::size_t rows);
};
