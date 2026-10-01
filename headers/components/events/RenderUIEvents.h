#pragma once

#include "components/UiTable.h"
#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include <vector>

struct MenuShowedEvent
{
	bool isShown;
};

struct ShowMenuEvent
{
	bool show;
};

struct ScoreBoardShowedEvent
{
	bool isDisplayed;
};

//NOTE: how far below its resting place the panel still is - only the menu slides in
struct RenderMenuBackgroundEvent
{
	int slide{};
};

//NOTE: tables stacked top to bottom in the middle of the panel, each centered across it, one font size for all
struct RenderPanelTablesEvent
{
	std::vector<UiTable> tables{};
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


//NOTE: a plate in the middle of the field - the pause, or the end of a match nobody shows a scoreboard for
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
