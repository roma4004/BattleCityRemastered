#include "components/UiLayout.h"
#include "components/UiTable.h"
#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include <algorithm>
#include <cstddef>
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
}//namespace

UiLayout::Placement UiLayout::Place(const UiTable& table, const Point origin, const int rowHeight,
									const Measure& measure)
{
	std::vector<std::vector<Point>> sizes{};
	std::vector<int> columnWidths{};
	sizes.reserve(table.rows.size());

	for (const UiRow& row: table.rows)
	{
		std::vector<Point> rowSizes{};
		rowSizes.reserve(row.cells.size());
		columnWidths.resize(std::max(columnWidths.size(), row.cells.size()));

		for (std::size_t column{}; column < row.cells.size(); ++column)
		{
			const Point size{measure(row.cells[column])};
			columnWidths[column] = std::max(columnWidths[column], size.x);
			rowSizes.push_back(size);
		}

		sizes.push_back(std::move(rowSizes));
	}

	std::vector<int> columnStarts(columnWidths.size());
	int tableWidth{};
	for (std::size_t column{}; column < columnWidths.size(); ++column)
	{
		columnStarts[column] = tableWidth;
		tableWidth += columnWidths[column] + (column + 1 < columnWidths.size() ? kColumnGap : 0);
	}

	Placement placement{};
	placement.size = Point{.x = tableWidth, .y = static_cast<int>(table.rows.size()) * rowHeight};
	placement.rows.reserve(table.rows.size());

	for (std::size_t row{}; row < table.rows.size(); ++row)
	{
		const int rowTop{origin.y + static_cast<int>(row) * rowHeight};
		placement.rows.push_back(Point{.x = origin.x, .y = rowTop});

		for (std::size_t column{}; column < table.rows[row].cells.size(); ++column)
		{
			const UiCell& cell{table.rows[row].cells[column]};
			const Point size{sizes[row][column]};
			const Point pos{.x = origin.x + columnStarts[column] + AlignedX(cell, columnWidths[column], size.x)
								 + cell.offset.x,
							.y = rowTop + (rowHeight - size.y) / 2 + cell.offset.y};

			placement.cells.push_back(PlacedCell{.pos = pos,
												 .size = size,
												 .row = static_cast<int>(row),
												 .column = static_cast<int>(column)});
		}
	}

	return placement;
}
