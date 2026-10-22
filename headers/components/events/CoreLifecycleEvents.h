#pragma once

#include "components/MatchSettings.h"
#include "geometry/Point.h"
#include "enums/Absence.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include "enums/PortForwarding.h"
#include "enums/TankModel.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct ServerInClientReadyToStartGameEvent
{
	PlayerSlot slot;
};

//NOTE: a client's driving key; a player watching a bot in their seat takes the tank over with it
struct ServerInTankCommandEvent
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
	//NOTE: the match the host was started for - its seat count, and what the lobby shows
	MatchSettings match{};
};

//NOTE: the server's port on the router, as the server tells every seat - the address a friend dials from the internet
struct PortForwardingChangedEvent
{
	PortForwarding state{};
	std::string host{};
	std::uint16_t port{};
};

//NOTE: the link dropped without a goodbye - an announced leave is ServerInDisconnectEvent
struct ServerClientLostEvent
{
	PlayerSlot slot;
};

//NOTE: a client got its seat - it is not ready yet, but a match starting at once still waits for it
struct ServerClientSeatedEvent
{
	PlayerSlot slot;
};

//NOTE: the client has the field the server sent it, and the match may go on
struct ServerClientSyncedEvent
{
	PlayerSlot slot;
};

//NOTE: who sits where as a network match starts - the reset that follows spawns by it
struct SeatsFilledEvent
{
	std::array<SeatHolder, kSeatCount> holders{};
};

//NOTE: a running match's seat changed hands - a player sat down, or a leaver gave it to a bot or to nobody
struct SeatHolderChangedEvent
{
	PlayerSlot slot;
	SeatHolder from{};
	SeatHolder to{SeatHolder::Player};
};

//NOTE: who holds every seat, as the host tells the clients
struct SeatHoldersAnnouncedEvent
{
	std::array<SeatHolder, kSeatCount> holders{};
};

//NOTE: who left the running match and who is back since - while any seat says so, the match is held
struct AbsenceChangedEvent
{
	std::array<Absence, kSeatCount> seats{};
};

//NOTE: a player still in answers the panel - on the server, whoever sent it
struct AbsenceChosenEvent
{
	AbsenceChoice choice;
};

struct GameStateChangedToEvent
{
	GameState state;
};

struct MatchStartedEvent {};

//NOTE: not ResetEvent - <windows.h>, pulled in through SDL, declares a global function of that name
struct GameResetEvent
{
	//NOTE: a level change, not a restart - players keep lives and tier, enemies and statistics start over
	bool keepsPlayerProgress{};
};

struct LoadMapEvent {};

//NOTE: sent by the scoreboard, handled by the authority only - a client forwards it so both load the same map
struct NextLevelRequestedEvent {};

//NOTE: local ServerInRestartRequestedEvent - the seats are taken, so the match restarts without a new ready round
struct MatchRestartRequestedEvent {};

struct ClientOutReadyToPlayEvent {};

struct ClientOutRestartMatchEvent {};

struct FrameStartEvent {};

struct PresentFrameEvent {};

struct SendNetworkFrameEvent {};

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
	//NOTE: the file's name without .map - what a client joining later is told it plays on
	std::string name{};
};

//NOTE: read on the host only - a client gets each model with its tank and the count with the snapshot
struct EnemyLineupLoadedEvent
{
	//NOTE: none when the map does not say
	std::optional<std::size_t> count{};
	//NOTE: the first enemies' models - past them every one is rolled
	std::vector<TankModel> models{};
};

//NOTE: the reason is already logged; the field stayed empty, so whoever started the match must undo it
struct MapLoadFailedEvent {};

struct WorldGeometryChangedEvent {};

//NOTE: render target textures lost their pixels but stay valid; SDL only reports it
struct RenderTargetsResetEvent {};

//NOTE: SDL sends it only on device loss; code that replaces the renderer emits it too
struct RenderDeviceResetEvent {};
