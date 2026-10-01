#pragma once

#include "components/UiTable.h"
#include "geometry/Point.h"
#include <cstddef>
#include <functional>
#include <span>
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
	[[nodiscard]] static std::vector<Placement> MeasureAll(std::span<const UiTable> tables, int rowHeight,
														   const Measure& measure);
	//NOTE: measured once, then moved to where its width sits in the middle - no second layout
	[[nodiscard]] static Placement CenteredAcross(Placement placement, Point origin, int width);
	//NOTE: the tables one under another, with a gap between each two
	[[nodiscard]] static int StackHeight(std::span<const Placement> placed, int gap);
	//NOTE: only the words answer to the font - a picture is not asked, nor a table of nothing but pictures
	[[nodiscard]] static bool FitsAcross(std::span<const UiTable> tables, std::span<const Placement> placed, int width,
										 int rowHeight);
	//NOTE: the largest size from smallest to largest that goesIn accepts, smallest when none is
	[[nodiscard]] static int FitPointSize(int smallest, int largest, const std::function<bool(int)>& goesIn);
};
