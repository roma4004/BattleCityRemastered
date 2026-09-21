#pragma once

#include "enums/TextBlockAlign.h"
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

struct MenuPosChangedEvent
{
	Point pos;
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

//NOTE: how far the renderer moved the menu block to centre it in the panel - whoever lays something out
//over the menu takes the same number, because nothing outside the renderer can measure the text
struct MenuContentShiftedEvent
{
	int shiftX;
};

struct RenderMenuLogoEvent
{
	Point pos;
};

struct RenderMenuSelectorIconEvent
{
	Point pos;
};

struct RenderMenuXBoxHintEvent
{
	Point pos;
};

struct RenderMenuPS5HintEvent
{
	Point pos;
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
