#pragma once

#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include <string>
#include <utility>
#include <vector>

//NOTE: Default is what the cell is: a word starts at the left of its column, a picture stands in its middle
enum class UiAlign : char8_t
{
	Default,
	Left,
	Centered,
	Right
};

//NOTE: a word or a picture, never both, over an optional background - an empty cell is a spacer holding a column
struct UiCell final
{
	std::string text{};
	UiIcon icon{UiIcon::None};
	//NOTE: fills the cell's whole slot and is drawn first, so the word lands on it
	UiIcon background{UiIcon::None};
	unsigned int color{};
	UiAlign align{};
	//NOTE: a nudge off the place the row gives it - a picture a few pixels aside, a word onto its background's spot
	Point offset{};

	[[nodiscard]] bool operator==(const UiCell& rhs) const = default;
};

//NOTE: a word in the color of the screen it stands on - what nearly every cell is
[[nodiscard]] inline UiCell TextCell(std::string text, const unsigned int color, const UiAlign align = UiAlign::Default)
{
	return UiCell{.text = std::move(text), .color = color, .align = align};
}

struct UiRow final
{
	std::vector<UiCell> cells{};

	[[nodiscard]] bool operator==(const UiRow& rhs) const = default;
};

//NOTE: what is shown, with no pixel in it - rows of cells that stand in columns. Only the side that
//measures the text can say how wide a column is, so the pixels are worked out there.
struct UiTable final
{
	std::vector<UiRow> rows{};

	[[nodiscard]] bool operator==(const UiTable& rhs) const = default;
};
