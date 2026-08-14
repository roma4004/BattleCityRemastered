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
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &GameStateManager::Draw));
	_subs.push_back(_events->AddListener(this, &GameStateManager::Reset));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnPlayersTeamIsWon));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnEnemiesTeamIsWon));
	_subs.push_back(_events->AddListener(this, &GameStateManager::OnGameModeChangedTo));
}

void GameStateManager::OnPauseStatus(const PauseStatusEvent& event) { _isPause = event.isPaused; }

void GameStateManager::OnPlayersTeamIsWon(const PlayersTeamIsWonEvent&) { _isGameWon = true; }

void GameStateManager::OnEnemiesTeamIsWon(const EnemiesTeamIsWonEvent&) { _isGameOver = true; }

void GameStateManager::OnGameModeChangedTo(const GameModeChangedToEvent& event) { _gameMode = event.mode; }

void GameStateManager::Draw(const PreDrawUserInterfaceEvent&) const
{
	if (_isPause)
	{
		_events->EmitEvent(RenderPauseTextEvent{});
	}

	if (_isGameOver)
	{
		_events->EmitEvent(RenderGameOverTextEvent{});
	}

	if (_isGameWon)
	{
		_events->EmitEvent(RenderGameWonTextEvent{});
	}
}

void GameStateManager::Reset(const GameResetEvent&)
{
	_isPause = false;
	_isGameOver = false;
	_isGameWon = false;
}
