#include "components/managers/GameStateManager.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "enums/GameMode.h"

GameStateManager::GameStateManager(const std::shared_ptr<EventSystem>& events)
	: _name{"GameStateManager"}
	, _events{events}
{
	Subscribe();
}

void GameStateManager::Subscribe()
{
	_subs.push_back(_events->AddListener(_name, [this](const PauseStatusEvent& event) { this->_isPause = event.isPaused; }));
	_subs.push_back(_events->AddListener(_name, [this](const PreDrawUserInterfaceEvent&) { this->Draw(); }));
	_subs.push_back(_events->AddListener(_name, [this](const GameResetEvent&) { this->Reset(); }));
	_subs.push_back(_events->AddListener(_name, [this](const PlayersTeamIsWonEvent&) { this->_isGameWon = true; }));
	_subs.push_back(_events->AddListener(_name, [this](const EnemiesTeamIsWonEvent&) { this->_isGameOver = true; }));
	_subs.push_back(_events->AddListener(_name, [this](const GameModeChangedToEvent& event)
	{
		this->_gameMode = event.mode;
	}));
}

void GameStateManager::Draw() const
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

void GameStateManager::Reset()
{
	_isPause = false;
	_isGameOver = false;
	_isGameWon = false;
}
