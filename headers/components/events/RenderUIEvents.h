#pragma once

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

//NOTE: one size for every line - per-line sizes would break the block apart
struct TextBlockLine
{
	Point pos{};
	unsigned int color{};
	std::string text{};
};

struct RenderMenuTextBlockEvent
{
	Point menuPos;
	int lineHeight;
	//NOTE: a centred block gets its positions from the panel, so its lines carry none
	bool isCentered;
	std::vector<TextBlockLine> lines;
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
