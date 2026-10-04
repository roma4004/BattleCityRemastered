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

//NOTE: a seat of a running match changed hands - a player sat down in it, or the one who left gave it up to a bot
//or to nobody
struct SeatHolderChangedEvent
{
	PlayerSlot slot;
	SeatHolder from{};
	SeatHolder to{SeatHolder::Player};
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

//NOTE: the reason is already in the log - this only says the world never got filled, so whoever started
//the match has to take it back
struct MapLoadFailedEvent {};

struct WorldGeometryChangedEvent {};

//NOTE: the handle survives, the pixels do not - SDL reports it, repairs nothing
struct RenderTargetsResetEvent {};

//NOTE: SDL sends it only on real device loss - whoever replaces the renderer emits it himself
struct RenderDeviceResetEvent {};
