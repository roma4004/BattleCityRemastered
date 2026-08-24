#pragma once

#include "enums/GameMode.h"

struct GameModeChangedToEvent
{
	GameMode mode;
};

struct GameModeAppliedEvent
{
	GameMode mode;
};

struct SelectedGameModeChangedToEvent
{
	GameMode mode;
};
