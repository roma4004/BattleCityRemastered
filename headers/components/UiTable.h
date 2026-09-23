#pragma once

#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include <string>
#include <vector>

//NOTE: Default is what the cell is: a word starts at the left of its column, a picture stands in its middle
enum class UiAlign : char8_t
{
	Default,
	Left,
	Centered,
	Right
};

//NOTE: one word or one picture, never both - an empty text with UiIcon::None is a spacer holding a column
struct UiCell final
{
	std::string text{};
	UiIcon icon{UiIcon::None};
	unsigned int color{};
	UiAlign align{};
	//NOTE: nudge off the place the row gives it, for a picture that reads better a few pixels aside
	Point offset{};
};

struct UiRow final
{
	std::vector<UiCell> cells{};
};

//NOTE: what is shown, with no pixel in it - rows of cells that stand in columns. Only the side that
//measures the text can say how wide a column is, so the pixels are worked out there.
struct UiTable final
{
	std::vector<UiRow> rows{};
};
