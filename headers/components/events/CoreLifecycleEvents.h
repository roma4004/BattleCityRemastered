#pragma once

#include "geometry/Point.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include <cstddef>

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

struct MatchStartedEvent {};

//NOTE: named GameResetEvent, not ResetEvent - <windows.h> (pulled in transitively via SDL on this
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

struct DemoStartedEvent {};

struct PlayersBaseFinishedEvent {};

struct GameFinishedEvent
{
	GameState state{};
};

struct RespawnTanksEvent {};

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

struct WorldGeometryChangedEvent {};

//NOTE: the handle survives, the pixels do not - SDL reports it, repairs nothing
struct RenderTargetsResetEvent {};

//NOTE: SDL sends it only on real device loss - whoever replaces the renderer emits it himself
struct RenderDeviceResetEvent {};
