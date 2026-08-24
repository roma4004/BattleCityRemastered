#pragma once

#include "geometry/Point.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"

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

struct ClientReconnectAbandonedEvent {};

//NOTE: the host replays the world right behind this, so stale objects have to go
struct ClientConnectedToHostEvent {};

//NOTE: separate from ServerInDisconnectEvent, which stays what it says it is - an announced leave
struct ServerClientLostEvent {};

struct GameStateChangedToEvent
{
	GameState state;
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

struct PresentFrameEvent {};

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

//NOTE: the map's own size, not the window's - this is what the geometry is fitted to
struct MapLoadedEvent
{
	std::size_t cols;
	std::size_t rows;
};

//NOTE: carries both sizes because whoever already stands on the field has to be rescaled by their
//ratio - the new size alone does not say by how much
struct WorldGeometryChangedEvent
{
	float cellSize;
	float previousCellSize;
};
