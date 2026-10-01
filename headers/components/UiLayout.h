#pragma once

#include "components/UiTable.h"
#include "geometry/Point.h"
#include <cstddef>
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
		//NOTE: the whole slot of its column and row - what a background fills
		Point boxPos{};
		Point boxSize{};
		std::size_t row{};
		std::size_t column{};
	};

	struct Placement final
	{
		std::vector<PlacedCell> cells{};
		//NOTE: the top left of every row, each as wide as the table - what a clickable row is built from
		std::vector<Point> rows{};
		Point size{};

		//NOTE: the same places moved as one - a measured table goes where it belongs without being laid out again
		void ShiftBy(Point offset);
	};

	using Measure = std::function<Point(const UiCell&)>;

	static constexpr int kColumnGap{20};

	//NOTE: a row is as tall as its tallest cell, and never lower than minRowHeight
	[[nodiscard]] static Placement Place(const UiTable& table, Point origin, int minRowHeight, const Measure& measure,
										 int columnGap = kColumnGap);
};
