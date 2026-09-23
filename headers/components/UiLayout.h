#pragma once

#include "components/UiTable.h"
#include "geometry/Point.h"
#include <functional>
#include <vector>

//NOTE: turns a table into places. All it knows of a cell comes from the measurer, so the same code lays
//out the menu on screen and a table of known widths in a test.
class UiLayout final
{
public:
	struct PlacedCell final
	{
		Point pos{};
		Point size{};
		int row{};
		int column{};
	};

	struct Placement final
	{
		std::vector<PlacedCell> cells{};
		//NOTE: the top left of every row, each as wide as the table - what a clickable row is built from
		std::vector<Point> rows{};
		Point size{};
	};

	using Measure = std::function<Point(const UiCell&)>;

	static constexpr int kColumnGap{20};

	[[nodiscard]] static Placement Place(const UiTable& table, Point origin, int rowHeight,
										 const Measure& measure);
};
