#pragma once

#include "enums/PlayerSlot.h"

struct ServerStatusRequestedEvent {};
struct ServerPlayersRequestedEvent {};

struct ServerKickRequestedEvent
{
	PlayerSlot slot;
};

struct ServerAcceptingChangedEvent
{
	bool isAccepting;
};
