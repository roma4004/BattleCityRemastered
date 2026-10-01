#include "components/managers/GameStateManager.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/WorldSnapshot.h"
#include "enums/UiIcon.h"
#include "enums/PlayerSlot.h"
#include <cstddef>

GameStateManager::GameStateManager(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

void GameStateManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnGameModeApplied));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnDemoStarted));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &GameStateManager::Draw));
	_subs.push_back(_events->AddListener(this, &GameStateManager::DrawOverScoreBoard));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnScoreBoardShowed));
	_subs.push_back(_events->AddListener(this, &GameStateManager::Reset));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnGameFinished));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnMapLoadFailed));

	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientReady));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnRestartRequested));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnMatchRestartRequested));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostPhase));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientLeft));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientLost));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostLeft));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostUnreachable));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostLost));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnWorldSnapshotRequested));
}

void GameStateManager::SetState(const GameState state)
{
	if (_state == state)
	{
		return;
	}

	_state = state;
	AnnouncePhase();
}

void GameStateManager::AnnouncePhase() const
{
	_events->EmitEvent(GameStateChangedToEvent{.state = _state});

	if (_state == GameState::Playing)
	{
		_events->EmitEvent(MatchStartedEvent{});
	}
}

void GameStateManager::Resume()
{
	_state = GameState::Playing;
	_events->EmitEvent(GameStateChangedToEvent{.state = _state});
}

GameState GameStateManager::IdleStateForMode() const
{
	if (_isDemo)
	{
		return GameState::Demo;
	}

	if (!IsNetworkGame(_gameMode))
	{
		return GameState::Playing;
	}

	//NOTE: only the host counts seats. A client waits to be told, so its own idle phase is the lobby
	return IsHost(_gameMode) && _readySeats.all() ? GameState::Playing : GameState::Lobby;
}

void GameStateManager::OnGameModeApplied(const GameModeAppliedEvent& event)
{
	_gameMode = event.mode;
	_readySeats.reset();
	_isDemo = false;

	//NOTE: announced even when the phase keeps its name - spawners act on entering one, not on a diff
	_state = IdleStateForMode();
	AnnouncePhase();
}

void GameStateManager::OnDemoStarted(const DemoStartedEvent&)
{
	_isDemo = true;
	SetState(GameState::Demo);
}

void GameStateManager::OnPauseStatus(const PauseStatusEvent& event)
{
	_isPaused = event.isPaused;

	if (event.isPaused && _state == GameState::Playing)
	{
		SetState(GameState::Paused);
	}
	else if (!event.isPaused && _state == GameState::Paused)
	{
		Resume();
	}
}

void GameStateManager::OnGameFinished(const GameFinishedEvent& event) { SetState(event.state); }

//NOTE: not the mode's idle phase - a lobby would start the same match the moment the seats are ready,
//and it would fail on the same map
void GameStateManager::OnMapLoadFailed(const MapLoadFailedEvent&) { SetState(GameState::Menu); }

void GameStateManager::TakeSeat(const PlayerSlot slot)
{
	_readySeats.set(static_cast<std::size_t>(slot));

	if (_state == GameState::Lobby && _readySeats.all())
	{
		SetState(GameState::Playing);
	}
}

void GameStateManager::FreeSeat(const PlayerSlot slot)
{
	_readySeats.reset(static_cast<std::size_t>(slot));

	if (!IsNetworkGame(_gameMode))
	{
		return;
	}

	//NOTE: the match goes on for whoever stayed - the one who comes back catches up from a snapshot
	if (_readySeats.any() && IsInMatch(_state))
	{
		return;
	}

	SetState(GameState::Lobby);
}

void GameStateManager::LoseHost()
{
	if (IsNetworkGame(_gameMode))
	{
		SetState(GameState::Lobby);
	}
}

void GameStateManager::OnClientReady(const ServerInClientReadyToStartGameEvent& event) { TakeSeat(event.slot); }

//NOTE: announced rather than set - a local match is already Playing, and SetState would see no change
//and leave the finished board on the screen
void GameStateManager::OnMatchRestartRequested(const MatchRestartRequestedEvent&)
{
	_state = IdleStateForMode();
	AnnouncePhase();
}

void GameStateManager::OnRestartRequested(const ServerInRestartRequestedEvent&)
{
	_readySeats.reset();
	_state = GameState::Lobby;
	AnnouncePhase();
}

//NOTE: announced even when the phase keeps its name - SetState would swallow the repeat
void GameStateManager::OnHostPhase(const HostPhaseAnnouncedEvent& event)
{
	//NOTE: the field the players are looking at is not restarted under them - leaving a pause announces
	//Playing again, and a full announce would have it emptied and the map loaded over
	if (const bool isResuming{event.phase == GameState::Playing
							  && (_state == GameState::Paused || _state == GameState::Playing)};
		isResuming)
	{
		Resume();

		return;
	}

	_state = event.phase;
	AnnouncePhase();
}

void GameStateManager::OnClientLeft(const ServerInDisconnectEvent& event) { FreeSeat(event.slot); }
void GameStateManager::OnClientLost(const ServerClientLostEvent& event) { FreeSeat(event.slot); }
void GameStateManager::OnHostLeft(const ClientInDisconnectEvent&) { LoseHost(); }
void GameStateManager::OnHostUnreachable(const ClientReconnectAbandonedEvent&) { LoseHost(); }
void GameStateManager::OnHostLost(const ClientHostLostEvent&) { LoseHost(); }

void GameStateManager::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	event.snapshot.phase = _state;
}

void GameStateManager::OnScoreBoardShowed(const ScoreBoardShowedEvent& event)
{
	_isScoreBoardShown = event.isDisplayed;
}

bool GameStateManager::IsPauseShown() const
{
	return _state == GameState::Paused || (_isPaused && (IsInMatch(_state) || _state == GameState::Demo));
}

//NOTE: the pause only - the end of a match is the scoreboard's, which shows its plate on itself or in the field
void GameStateManager::Draw(const PreDrawUserInterfaceEvent&) const
{
	if (IsPauseShown() && !_isScoreBoardShown)
	{
		_events->EmitEvent(RenderPlateEvent{.plate = UiIcon::PlatePause});
	}
}

//NOTE: after the board, not before - its see-through panel would lay the statistics over the plate
void GameStateManager::DrawOverScoreBoard(const PostDrawUserInterfaceEvent&) const
{
	if (IsPauseShown() && _isScoreBoardShown)
	{
		_events->EmitEvent(RenderPlateEvent{.plate = UiIcon::PlatePause});
	}
}

void GameStateManager::Reset(const GameResetEvent&)
{
	if (_state == GameState::Won || _state == GameState::Over)
	{
		SetState(IdleStateForMode());
	}
}
