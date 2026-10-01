#pragma once

#include "components/UiTable.h"
#include "geometry/Point.h"
#include <cstddef>
#include <functional>
#include <optional>
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
	};

	using Measure = std::function<Point(const UiCell&)>;

	static constexpr int kColumnGap{20};

	//NOTE: without a row height every row is as tall as its tallest cell
	[[nodiscard]] static Placement Place(const UiTable& table, Point origin, std::optional<int> rowHeight,
										 const Measure& measure, int columnGap = kColumnGap);
};
