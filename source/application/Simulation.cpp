#include "utils/MathUtils.h"
#include "application/Simulation.h"
#include "utils/Log.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/AnimationManager.h"
#include "components/managers/BonusManager.h"
#include "components/managers/GameStateManager.h"
#include "components/managers/SpawnManager.h"
#include "components/managers/WorldScaleManager.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "network/ClientNode.h"
#include "network/ServerNode.h"
#include "utils/TimeUtils.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

namespace
{
//NOTE: movement is speed * deltaTime, so a frame-length step lands the same input differently per machine
constexpr double kFixedStep{1.0 / 60.0};

constexpr double kStepSnapTolerance{kFixedStep / 20.0};

//NOTE: every step is a full frame of movement and shooting - a slow stretch must not come back as a burst
constexpr double kMaxCatchUpSteps{4.0};
}//namespace

Simulation::Simulation(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig)
	: _events{events}
	, _stateManager{std::make_unique<GameStateManager>(events, gameConfig.HostedMatch())}
	, _animationManager{std::make_unique<AnimationManager>(events)}
	, _statistics{std::make_unique<GameStatistics>(events)}
	, _worldScaleManager{std::make_unique<WorldScaleManager>(events, gameConfig)}
	, _spawnManager{std::make_unique<SpawnManager>(events, gameConfig)}
	, _bonusManager{std::make_unique<BonusManager>(events, gameConfig)}
	, _gameConfig{gameConfig}
{
	Subscribe();
}

Simulation::~Simulation() = default;

void Simulation::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Simulation::OnDeltaTime));
	_subs.push_back(_events->AddListener(this, &Simulation::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &Simulation::OnPostTickUpdate));
	_subs.push_back(_events->AddListener(this, &Simulation::OnGameStateChangedTo));
	_subs.push_back(_events->AddListener(this, &Simulation::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &Simulation::OnMatchStarted));
	_subs.push_back(_events->AddListener(this, &Simulation::OnNextLevelRequested));
	_subs.push_back(_events->AddListener(this, &Simulation::OnMapLoadFailed));
	_subs.push_back(_events->AddListener(this, &Simulation::OnConnectedToHost));
	_subs.push_back(_events->AddListener(this, &Simulation::OnPlayerSlotAssigned));
	_subs.push_back(_events->AddListener(this, &Simulation::OnHostLeft));
	_subs.push_back(_events->AddListener(this, &Simulation::OnHostUnreachable));
	_subs.push_back(_events->AddListener(this, &Simulation::OnHostLost));
}

const std::vector<std::shared_ptr<BaseObj>>& Simulation::World() const { return _spawnManager->World(); }

const AnimationManager& Simulation::Animations() const { return *_animationManager; }

const GameStateManager& Simulation::State() const { return *_stateManager; }

const GameStatistics& Simulation::Statistics() const { return *_statistics; }

void Simulation::OnDeltaTime(const DeltaTimeEvent& event) { _deltaTime = event.deltaTime; }

void Simulation::OnPauseStatus(const PauseStatusEvent& event) { _isPaused = event.isPaused; }

void Simulation::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	if (_isEnterLobbyPending)
	{
		_isEnterLobbyPending = false;
		EnterLobby();
	}

	if (_isNextLevelPending)
	{
		_isNextLevelPending = false;
		StartNextLevel();
	}
}

void Simulation::OnGameStateChangedTo(const GameStateChangedToEvent& event)
{
	_gameConfig.gameState = event.state;

	_isEnterLobbyPending = event.state == GameState::Lobby;
}

void Simulation::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	//NOTE: reset first - assigning over it would build the new node while the old one still holds the port
	_networkNode.reset();

	_boundPort = 0u;

	if (IsHost(event.mode))
	{
		auto server{std::make_unique<network::ServerNode>(_gameConfig.serverAddress, _events,
																	_gameConfig.HostedMatch())};
		//NOTE: asked for port 0, the OS picked one - read it here, while the concrete type is still in hand
		_boundPort = server->GetBoundPort();
		_networkNode = std::move(server);
	}
	else if (IsClient(event.mode))
	{
		//NOTE: port 0 is for servers only - a client waits until Game finds a published port and comes back here
		if (_gameConfig.serverAddress.port == network::kAnyFreePort)
		{
			Log::Info("no server port to join yet - waiting for the server to tell it");

			return;
		}

		_networkNode = std::make_unique<network::ClientNode>(_gameConfig.serverAddress, _events);
	}
}

//NOTE: the map goes last so a failed load can bring the menu back
void Simulation::OnMatchStarted(const MatchStartedEvent&)
{
	if (!_isNextLevel)
	{
		_gameConfig.stageNumber = 1u;
	}

	_events->EmitEvent(GameResetEvent{.keepsPlayerProgress = _isNextLevel});
	_isNextLevel = false;

	_events->EmitEvent(ShowMenuEvent{.isShown = false});
	_events->EmitEvent(SetPauseEvent{.isPaused = false});

	_events->EmitEvent(LoadMapEvent{});
}

//NOTE: a client only asks over the wire; the authority's phase change brings both machines to the same map
void Simulation::OnNextLevelRequested(const NextLevelRequestedEvent&)
{
	if (IsClient(_gameConfig.gameMode))
	{
		return;
	}

	_isNextLevelPending = true;
}

//NOTE: the map is chosen before the teardown; the following reset keeps the players' progress from the won level
void Simulation::StartNextLevel()
{
	_gameConfig.mapPath = _levels.PathAfter(_gameConfig.mapPath);
	++_gameConfig.stageNumber;
	_isNextLevel = true;

	Log::Info("next level runs on " + _gameConfig.mapPath);

	if (IsHost(_gameConfig.gameMode))
	{
		_events->EmitEvent(ServerInRestartRequestedEvent{});

		return;
	}

	//NOTE: the reset comes with the next match - two resets in one dispatch would load a map, then throw it away
	_events->EmitEvent(MatchRestartRequestedEvent{});
}

void Simulation::OnMapLoadFailed(const MapLoadFailedEvent&) const
{
	_events->EmitEvent(ShowMenuEvent{.isShown = true});
}

void Simulation::OnConnectedToHost(const ClientConnectedToHostEvent&)
{
	_isLinkUp = true;

	//NOTE: not announced here - the ready follows the reset, and EnterLobby is the one place doing both
	_isEnterLobbyPending = _gameConfig.gameState == GameState::Lobby;
}

void Simulation::OnPlayerSlotAssigned(const PlayerSlotAssignedEvent& event)
{
	_gameConfig.ownSlot = event.slot;
	_gameConfig.networkSeats = event.match.seats;
}

void Simulation::OnHostLeft(const ClientInDisconnectEvent&) { _isLinkUp = false; }

void Simulation::OnHostUnreachable(const ClientReconnectAbandonedEvent&) { _isLinkUp = false; }

void Simulation::OnHostLost(const ClientHostLostEvent&) { _isLinkUp = false; }

//NOTE: the mode is kept - dropping it tears the link down, and nobody could reconnect
void Simulation::EnterLobby()
{
	//NOTE: keeps progress too - on a host the lobby sits between the won level and the next map
	_events->EmitEvent(GameResetEvent{.keepsPlayerProgress = _isNextLevel});
	_events->EmitEvent(ShowMenuEvent{.isShown = false});

	if (_isLinkUp)
	{
		AnnounceReady();
	}
}

//NOTE: after the reset, never before - the host answers with the world, and the reset would erase it
void Simulation::AnnounceReady() const { _events->EmitEvent(ClientOutReadyToPlayEvent{}); }

void Simulation::LeaveGameMode() { _networkNode.reset(); }

void Simulation::ApplyGameMode(const GameMode gameMode)
{
	//NOTE: before the reset - GameResetEvent listeners read the new mode from the config
	_gameConfig.gameMode = gameMode;

	_events->EmitEvent(GameResetEvent{});

	_isLinkUp = false;
	_gameConfig.ownSlot.reset();//NOTE: a new link is a new seat, and it may not be the old one
	_events->EmitEvent(GameModeChangedToEvent{.mode = gameMode});
	_events->EmitEvent(GameModeAppliedEvent{.mode = gameMode});
}

//NOTE: a restart is a wire command, not a re-entry - rebuilding the mode would drop a working link
bool Simulation::TryRestartMatch() const
{
	if (!_isLinkUp)
	{
		return false;
	}

	_events->EmitEvent(ClientOutRestartMatchEvent{});

	return true;
}

void Simulation::Tick()
{
	_events->EmitEvent(FrameStartEvent{});
	_events->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTime});
	_events->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTime});

	//NOTE: after PreTickUpdate, which polls the keyboard - a pause pressed this frame applies this frame
	const bool isRunning{!_isPaused && _gameConfig.gameState != GameState::Lobby};
	if (isRunning)
	{
		TimeUtils::Advance(
				std::chrono::duration_cast<TimeUtils::clock::duration>(std::chrono::duration<double>{_deltaTime}));
	}

	if (isRunning && _gameConfig.IsAuthority())
	{
		_events->EmitEvent(RespawnTanksEvent{});//NOTE: on the wall clock, so once a frame

		const double elapsed{
				MathUtils::AreEqualAbsolute(_deltaTime, kFixedStep, kStepSnapTolerance) ? kFixedStep : _deltaTime};
		_stepAccumulator = std::min(_stepAccumulator + elapsed, kFixedStep * kMaxCatchUpSteps);

		while (_stepAccumulator >= kFixedStep)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = kFixedStep});
			_stepAccumulator -= kFixedStep;
		}
	}

	_events->EmitEvent(PostTickUpdateEvent{.deltaTime = _deltaTime});
}

void Simulation::SendNetworkFrame() const
{
	if (IsNetworkGame(_gameConfig.gameMode))
	{
		_events->EmitEvent(SendNetworkFrameEvent{});
	}
}
