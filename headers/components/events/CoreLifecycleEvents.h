#pragma once

#include "geometry/Point.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
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

//NOTE: which seat this process drives - the server hands it out on connect
struct PlayerSlotAssignedEvent
{
	PlayerSlot slot;
};

//NOTE: the link dropped without a goodbye - an announced leave is ServerInDisconnectEvent
struct ServerClientLostEvent {};

struct GameStateChangedToEvent
{
	GameState state;
};

struct MatchStartedEvent {};

//NOTE: named GameResetEvent because <windows.h>, pulled in through SDL, declares a function literally
//called `ResetEvent` - a same-named struct in the global namespace collides with it
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
