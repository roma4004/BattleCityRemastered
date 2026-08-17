#pragma once

#include "Point.h"
#include "enums/DisconnectReason.h"

struct ServerInClientReadyToStartGameEvent {};

//NOTE: only a deliberate leave produces these - a dropped link never does
struct ServerInDisconnectEvent
{
	DisconnectReason reason;
};

struct ClientInDisconnectEvent
{
	DisconnectReason reason;
};

//NOTE: named GameResetEvent, not ResetEvent - <windows.h> (pulled in transitively via SDL2 on this
//MinGW/Windows toolchain) declares a WinAPI function literally named `ResetEvent` (synchapi.h),
//which collides with a same-named struct in the global namespace (C++ tag names and function names
//share one namespace, unlike C). Every listener for the former "Reset" string event must use this
//struct, not a plain `ResetEvent`.
struct GameResetEvent {};

struct LoadMapEvent {};

struct ClientOutReadyToPlayEvent {};

struct FrameStartEvent {};

struct PreDrawEvent {};

struct DrawEvent {};

struct PostDrawEvent {};

struct PreDrawUserInterfaceEvent {};

struct DrawUserInterfaceEvent {};

struct PostDrawUserInterfaceEvent {};

struct NetworkEndFrameEvent {};

struct CalculateActualFpsEvent {};

struct PreviousGameModeEvent {};

struct NextGameModeEvent {};

struct ApplyGameModeEvent {};

struct PlayersBaseFinishedEvent {};

struct PlayersTeamIsWonEvent {};

struct EnemiesTeamIsWonEvent {};

struct ServerOutPlayersTeamIsWonEvent {};

struct ServerOutEnemiesTeamIsWonEvent {};

struct RespawnTanksEvent
{
	bool skipDelay;
};

struct ScaleFactorChangedToEvent
{
	float scale;
};


struct WindowSizeChangedToEvent
{
	UPoint newSize;
};
