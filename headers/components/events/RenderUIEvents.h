#pragma once

#include "Point.h"

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

struct RenderRightSideBarEvent {};

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
