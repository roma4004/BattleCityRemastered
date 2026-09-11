#include "application/Simulation.h"
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
#include <cmath>
#include <memory>

namespace
{
//NOTE: movement is speed * deltaTime, so a step that follows the frame makes the same input land
//differently on every machine
constexpr double kFixedStep{1.0 / 60.0};

constexpr double kStepSnapTolerance{kFixedStep / 20.0};

//NOTE: every step is a full frame of movement and shooting - a slow stretch must not come back
//as a burst of them
constexpr double kMaxCatchUpSteps{4.0};
}//namespace

Simulation::Simulation(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig)
	: _events{events}
	, _stateManager{std::make_unique<GameStateManager>(events)}
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

//NOTE: after every subsystem above and before the spawners, which are built on the first mode change -
//so a reset announced here still reaches a spawner that does not exist yet
void Simulation::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Simulation::OnDeltaTime));
	_subs.push_back(_events->AddListener(this, &Simulation::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &Simulation::OnPostTickUpdate));
	_subs.push_back(_events->AddListener(this, &Simulation::OnGameStateChangedTo));
	_subs.push_back(_events->AddListener(this, &Simulation::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &Simulation::OnMatchStarted));
	_subs.push_back(_events->AddListener(this, &Simulation::OnConnectedToHost));
	_subs.push_back(_events->AddListener(this, &Simulation::OnPlayerSlotAssigned));
	_subs.push_back(_events->AddListener(this, &Simulation::OnHostLeft));
	_subs.push_back(_events->AddListener(this, &Simulation::OnHostUnreachable));
}

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
}

void Simulation::OnGameStateChangedTo(const GameStateChangedToEvent& event)
{
	_gameConfig.gameState = event.state;

	_isEnterLobbyPending = event.state == GameState::Lobby;
}

void Simulation::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	//NOTE: the old node goes first - assigning over it would build the new one (same port, new
	//connect) while the outgoing one still holds both
	_networkNode.reset();

	if (IsHost(event.mode))
	{
		_networkNode = std::make_unique<network::commands::ServerNode>(_events);
	}
	else if (IsClient(event.mode))
	{
		_networkNode = std::make_unique<network::commands::ClientNode>(_events);
	}
}

void Simulation::OnMatchStarted(const MatchStartedEvent&)
{
	_events->EmitEvent(GameResetEvent{});

	_events->EmitEvent(ShowMenuEvent{.show = false});
	_events->EmitEvent(SetPauseEvent{.isPaused = false});
}

void Simulation::OnConnectedToHost(const ClientConnectedToHostEvent&)
{
	_isLinkUp = true;

	//NOTE: not announced here - the ready follows the reset, and EnterLobby is the one place doing both
	_isEnterLobbyPending = _gameConfig.gameState == GameState::Lobby;
}

void Simulation::OnPlayerSlotAssigned(const PlayerSlotAssignedEvent& event) { _gameConfig.ownSlot = event.slot; }

void Simulation::OnHostLeft(const ClientInDisconnectEvent&) { _isLinkUp = false; }

void Simulation::OnHostUnreachable(const ClientReconnectAbandonedEvent&) { _isLinkUp = false; }

//NOTE: the mode is kept - dropping it tears the link down, and nobody could reconnect
void Simulation::EnterLobby()
{
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(ShowMenuEvent{.show = false});

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
	//NOTE: before the reset - listeners of GameResetEvent read the mode off the config, so it has
	//to be the new one already
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

	//NOTE: read after PreTickUpdate, not before - that is where the keyboard is polled, so a pause
	//pressed this frame takes effect this frame
	const bool isRunning = !_isPaused && _gameConfig.gameState != GameState::Lobby;
	TimeUtils::SetPaused(!isRunning);

	if (isRunning && _gameConfig.IsAuthority())
	{
		_events->EmitEvent(RespawnTanksEvent{});//NOTE: on the wall clock, so once a frame

		const double elapsed =
				std::abs(_deltaTime - kFixedStep) < kStepSnapTolerance ? kFixedStep : _deltaTime;
		_stepAccumulator = std::min(_stepAccumulator + elapsed, kFixedStep * kMaxCatchUpSteps);

		while (_stepAccumulator >= kFixedStep)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = kFixedStep});
			_stepAccumulator -= kFixedStep;
		}
	}

	_events->EmitEvent(PostTickUpdateEvent{.deltaTime = _deltaTime});
}

void Simulation::EndNetworkFrame() const
{
	if (IsNetworkGame(_gameConfig.gameMode))
	{
		_events->EmitEvent(NetworkEndFrameEvent{});
	}
}
