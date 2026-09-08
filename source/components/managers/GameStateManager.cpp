#include "components/managers/GameStateManager.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"

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
	_subs.push_back(_events->AddListener(this, &GameStateManager::Reset));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnGameFinished));

	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientReady));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnConnectedToHost));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientLeft));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnClientLost));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostLeft));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnHostUnreachable));
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

void GameStateManager::AnnouncePhase()
{
	if (_state == GameState::Playing)
	{
		_events->EmitEvent(MatchStartedEvent{});
	}

	_events->EmitEvent(GameStateChangedToEvent{.state = _state});
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

	return IsNetworkGame(_gameMode) && _peerCount < PeersToWaitFor() ? GameState::Lobby : GameState::Playing;
}

//NOTE: PlayAsHost is reached only inside BattleCityServer, which fills no seat itself, so it
//waits for both players. A client waits for one peer, and that peer is the server, not player two.
unsigned short GameStateManager::PeersToWaitFor() const { return IsHost(_gameMode) ? 2u : 1u; }

void GameStateManager::OnGameModeApplied(const GameModeAppliedEvent& event)
{
	_gameMode = event.mode;
	_peerCount = 0u;
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

void GameStateManager::PeerArrived()
{
	++_peerCount;

	if (_state == GameState::Lobby && _peerCount >= PeersToWaitFor())
	{
		SetState(GameState::Playing);
	}
}

void GameStateManager::PeerGone()
{
	if (_peerCount > 0u)
	{
		--_peerCount;
	}

	if (IsNetworkGame(_gameMode))
	{
		SetState(GameState::Lobby);
	}
}

void GameStateManager::OnClientReady(const ServerInClientReadyToStartGameEvent&) { PeerArrived(); }
void GameStateManager::OnConnectedToHost(const ClientConnectedToHostEvent&) { PeerArrived(); }
void GameStateManager::OnClientLeft(const ServerInDisconnectEvent&) { PeerGone(); }
void GameStateManager::OnClientLost(const ServerClientLostEvent&) { PeerGone(); }
void GameStateManager::OnHostLeft(const ClientInDisconnectEvent&) { PeerGone(); }
void GameStateManager::OnHostUnreachable(const ClientReconnectAbandonedEvent&) { PeerGone(); }

void GameStateManager::Draw(const PreDrawUserInterfaceEvent&) const
{
	switch (_state)
	{
		case GameState::Paused:
			_events->EmitEvent(RenderPauseTextEvent{});
			break;
		case GameState::Over:
			_events->EmitEvent(RenderGameOverTextEvent{});
			break;
		case GameState::Won:
			_events->EmitEvent(RenderGameWonTextEvent{});
			break;
		default:
			break;
	}
}

void GameStateManager::Reset(const GameResetEvent&)
{
	if (_state == GameState::Won || _state == GameState::Over)
	{
		SetState(IdleStateForMode());
	}
}
