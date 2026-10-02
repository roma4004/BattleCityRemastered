#pragma once

#include "components/UiTable.h"
#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

struct MenuShownEvent
{
	bool isShown;
};

//NOTE: the keyboard types while it is up
struct ServerScreenShownEvent
{
	bool isShown;
};

struct ShowMenuEvent
{
	bool isShown;
};

struct ScoreBoardShownEvent
{
	bool isShown;
};

//NOTE: how far below its resting place the panel still is - only the menu slides in
struct RenderMenuBackgroundEvent
{
	int slide{};
};

//NOTE: a run of the picked table's rows is a window over a list - one that fits keeps the bar's room, with no bar
struct PanelScroll
{
	std::size_t firstRow{};
	std::size_t rowCount{};
	//NOTE: counted in the list's own items - one may take more than a row
	std::size_t firstShown{};
	std::size_t shownCount{};
	std::size_t total{};
};

//NOTE: drawn over a picked table row's text, so it never moves it
struct PanelCaret
{
	std::size_t row{};
	std::size_t symbol{};
	std::uint8_t alpha{};
};

//NOTE: tables stacked top to bottom in the middle of the panel, each centered across it, one font size for all
struct RenderPanelTablesEvent
{
	std::vector<UiTable> tables{};
	//NOTE: the arrow marks its selected row, and its rows' places go out as PanelRowsPlacedEvent
	std::optional<std::size_t> pickedTable{};
	std::size_t selectedRow{};
	std::optional<PanelScroll> scroll{};
	std::optional<PanelCaret> caret{};
};

//NOTE: the whole menu in one ask - what travels is the rows themselves, because the panel they stand in,
//and with it every pixel, belongs to the renderer
struct RenderMenuEvent
{
	int slide{};
	int selectedRow{};
	UiTable title{};
	UiTable modes{};
	UiTable controls{};
};

//NOTE: where the clickable menu items ended up - the mouse has to hit them where they were drawn, and
//the point size that decided it is the renderer's own answer
struct MenuTilesPlacedEvent
{
	//NOTE: the top left of each tile, in the order the modes stand in
	std::vector<Point> tiles{};
	Point tileSize{};
};

//NOTE: the same for the rows of a picked panel table
struct PanelRowsPlacedEvent
{
	std::vector<Point> rows{};
	Point rowSize{};
};


//NOTE: a plate over the field - the pause, or the end of a match nobody shows a scoreboard for
struct RenderPlateEvent
{
	UiIcon plate{};
};

//NOTE: the side column as two tables - the reserve stands in its frame at the top, the counters under it
struct RenderSideBarEvent
{
	UiTable enemies{};
	UiTable counters{};
};

struct RenderFPSEvent
{
	unsigned int fps;
};
