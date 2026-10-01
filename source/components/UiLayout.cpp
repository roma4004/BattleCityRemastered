#include "components/UiLayout.h"
#include "components/UiTable.h"
#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace
{
constexpr auto IsPicture = [](const UiCell& cell) { return cell.icon != UiIcon::None; };

int AlignedX(const UiCell& cell, const int columnWidth, const int cellWidth)
{
	const UiAlign align{cell.align == UiAlign::Default ? (IsPicture(cell) ? UiAlign::Centered : UiAlign::Left)
													   : cell.align};
	switch (align)
	{
		case UiAlign::Centered:
			return (columnWidth - cellWidth) / 2;
		case UiAlign::Right:
			return columnWidth - cellWidth;
		default:
			return 0;
	}
}

//NOTE: the larger of the two on each axis; the offset is not counted, so a nudged word may run past its picture
Point BoxSize(const UiCell& cell, const Point size, const UiLayout::Measure& measure)
{
	if (cell.background == UiIcon::None)
	{
		return size;
	}

	const Point background{measure(UiCell{.icon = cell.background})};

	return Point{.x = std::max(size.x, background.x), .y = std::max(size.y, background.y)};
}
}//namespace

void UiLayout::Placement::ShiftBy(const Point offset)
{
	const auto shift = [offset](Point& point)
	{
		point.x += offset.x;
		point.y += offset.y;
	};

	for (PlacedCell& cell: cells)
	{
		shift(cell.pos);
		shift(cell.boxPos);
	}

	std::ranges::for_each(rows, shift);
}

UiLayout::Placement UiLayout::Place(const UiTable& table, const Point origin, const int minRowHeight,
									const Measure& measure, const int columnGap)
{
	std::vector<std::vector<Point>> sizes{};
	std::vector<int> columnWidths{};
	std::vector<int> rowHeights{};
	sizes.reserve(table.rows.size());
	rowHeights.reserve(table.rows.size());

	for (const UiRow& row: table.rows)
	{
		std::vector<Point> rowSizes{};
		rowSizes.reserve(row.cells.size());
		columnWidths.resize(std::max(columnWidths.size(), row.cells.size()));
		int tallest{minRowHeight};

		for (std::size_t column{}; column < row.cells.size(); ++column)
		{
			const Point size{measure(row.cells[column])};
			const Point box{BoxSize(row.cells[column], size, measure)};
			columnWidths[column] = std::max(columnWidths[column], box.x);
			tallest = std::max(tallest, box.y);
			rowSizes.push_back(size);
		}

		sizes.push_back(std::move(rowSizes));
		rowHeights.push_back(tallest);
	}

	std::vector<int> columnStarts(columnWidths.size());
	int tableWidth{};
	for (std::size_t column{}; column < columnWidths.size(); ++column)
	{
		columnStarts[column] = tableWidth;
		tableWidth += columnWidths[column] + (column + 1 < columnWidths.size() ? columnGap : 0);
	}

	Placement placement{};
	placement.rows.reserve(table.rows.size());

	int rowTop{origin.y};
	for (std::size_t row{}; row < table.rows.size(); ++row)
	{
		placement.rows.push_back(Point{.x = origin.x, .y = rowTop});

		for (std::size_t column{}; column < table.rows[row].cells.size(); ++column)
		{
			const UiCell& cell{table.rows[row].cells[column]};
			const Point size{sizes[row][column]};
			const Point pos{.x = origin.x + columnStarts[column] + AlignedX(cell, columnWidths[column], size.x)
								 + cell.offset.x,
							.y = rowTop + (rowHeights[row] - size.y) / 2 + cell.offset.y};

			placement.cells.push_back(PlacedCell{.pos = pos,
												 .size = size,
												 .boxPos = {.x = origin.x + columnStarts[column], .y = rowTop},
												 .boxSize = {.x = columnWidths[column], .y = rowHeights[row]},
												 .row = row,
												 .column = column});
		}

		rowTop += rowHeights[row];
	}

	placement.size = Point{.x = tableWidth, .y = rowTop - origin.y};

	return placement;
}
