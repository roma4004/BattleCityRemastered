#pragma once

#include "enums/TextBlockAlign.h"
#include "components/UiTable.h"
#include "geometry/Point.h"
#include <string>
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

struct RenderMenuBackgroundEvent
{
	Point pos;
};

//NOTE: no size of its own - the renderer fits one font size to the whole block
struct TextBlockLine
{
	Point pos{};
	unsigned int color{};
	std::string text{};

	[[nodiscard]] bool operator==(const TextBlockLine& rhs) const noexcept = default;
};

struct RenderMenuTextBlockEvent
{
	Point menuPos;
	int lineHeight;
	TextBlockAlign align{};
	std::vector<TextBlockLine> lines;
};

//NOTE: the whole menu in one ask - what travels is the rows themselves, because the panel they stand in,
//and with it every pixel, belongs to the renderer
struct RenderMenuEvent
{
	Point menuPos{};
	int selectedRow{};
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


struct RenderEnemyIconBackgroundEvent {};

struct RenderPauseTextEvent {};

struct RenderGameOverTextEvent {};

struct RenderGameWonTextEvent {};

struct RenderEnemyIconsEvent
{
	unsigned short count;
};

struct RenderPlayerOneIconEvent
{
	unsigned short respawnCount;
};

struct RenderPlayerTwoIconEvent
{
	unsigned short respawnCount;
};

struct RenderStageNumberEvent
{
	unsigned short stageNumber;
};

struct RenderFPSEvent
{
	unsigned int fps;
};
