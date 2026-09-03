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

//NOTE: one size for every line - per-line sizes would break the block apart
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

	//NOTE: the renderer keeps the last block to see whether it has to fit the font again
	[[nodiscard]] bool operator==(const RenderMenuTextBlockEvent& rhs) const noexcept = default;
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
