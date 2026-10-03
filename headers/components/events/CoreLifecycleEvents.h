#pragma once

#include "geometry/Point.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include "enums/TankModel.h"
#include <cstddef>
#include <optional>
#include <vector>

struct ServerInClientReadyToStartGameEvent
{
	PlayerSlot slot;
};

struct ServerInRestartRequestedEvent {};

//NOTE: only a deliberate leave produces these - a dropped link never does
struct ServerInDisconnectEvent
{
	DisconnectReason reason;
	PlayerSlot slot;
};

struct ClientInDisconnectEvent
{
	DisconnectReason reason;
};

struct ClientReconnectAbandonedEvent {};

struct ClientConnectedToHostEvent {};

//NOTE: the link dropped without a goodbye - the client keeps dialling, and its field is stale meanwhile
struct ClientHostLostEvent {};

struct HostPhaseAnnouncedEvent
{
	GameState phase;
};

//NOTE: which seat this process drives - the server hands it out on connect
struct PlayerSlotAssignedEvent
{
	PlayerSlot slot;
	//NOTE: the host's seat count; zero when not sent
	std::uint8_t seatCount{};
};

//NOTE: the link dropped without a goodbye - an announced leave is ServerInDisconnectEvent
struct ServerClientLostEvent
{
	PlayerSlot slot;
};

struct GameStateChangedToEvent
{
	GameState state;
};

struct MatchStartedEvent {};

//NOTE: named GameResetEvent because <windows.h>, pulled in through SDL, declares a function literally
//called `ResetEvent` - a same-named struct in the global namespace collides with it
struct GameResetEvent
{
	//NOTE: a level change goes the way a restart does, and this is what tells the two apart - the players
	//keep the lives and the tier they earned, the enemies and the statistics start over
	bool keepsPlayerProgress{};
};

struct LoadMapEvent {};

//NOTE: asked for by whoever shows the scoreboard, answered by the authority - a client sends it on
//the wire instead, or the two machines would walk off onto different maps
struct NextLevelRequestedEvent {};

//NOTE: the local twin of ServerInRestartRequestedEvent - the seats are already taken, so the match
//starts over where it stands instead of asking anyone to be ready again
struct MatchRestartRequestedEvent {};

struct ClientOutReadyToPlayEvent {};

struct ClientOutRestartMatchEvent {};

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
	unsigned short stage{};
};

//NOTE: read on the host only - a client gets each model with its tank and the count with the snapshot
struct EnemyLineupLoadedEvent
{
	//NOTE: none when the map does not say
	std::optional<std::size_t> count{};
	//NOTE: the first enemies' models - past them every one is rolled
	std::vector<TankModel> models{};
};

//NOTE: the reason is already in the log - this only says the world never got filled, so whoever started
//the match has to take it back
struct MapLoadFailedEvent {};

struct WorldGeometryChangedEvent {};

//NOTE: the handle survives, the pixels do not - SDL reports it, repairs nothing
struct RenderTargetsResetEvent {};

//NOTE: SDL sends it only on real device loss - whoever replaces the renderer emits it himself
struct RenderDeviceResetEvent {};
