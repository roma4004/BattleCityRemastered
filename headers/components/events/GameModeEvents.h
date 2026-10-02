#pragma once

#include "enums/GameMode.h"
#include "network/Endpoints.h"

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

//NOTE: the server screen's answer - the address to enter the mode with
struct ServerAddressChosenEvent
{
	GameMode mode;
	network::ServerAddress address;
};
