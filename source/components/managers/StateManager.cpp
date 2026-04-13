#include "components/managers/StateManager.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"

StateManager::StateManager(const std::shared_ptr<EventSystem>& events)
	: _name{"StateManager"}
	, _events{events}
{
	Subscribe();
}

StateManager::~StateManager() { Unsubscribe(); }

void StateManager::Subscribe()
{
	_events->AddListener("Pause_Status", _name, [this](const bool value) { this->_isPause = value; });
	_events->AddListener("PreDrawUserInterface", _name, [this]() { this->Draw(); });
	_events->AddListener("Reset", _name, [this]() { this->Reset(); });
	_events->AddListener("PlayersTeamIsWon", _name, [this]() { this->_isGameWon = true; });
	_events->AddListener("EnemiesTeamIsWon", _name, [this]() { this->_isGameOver = true; });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
	});
}

void StateManager::Unsubscribe() const
{
	_events->RemoveListener("Pause_Status", _name);
	_events->RemoveListener("PreDrawUserInterface", _name);
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("PlayersTeamIsWon", _name);
	_events->RemoveListener("EnemiesTeamIsWon", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
}

void StateManager::Draw() const
{
	if (_isPause)
	{
		_events->EmitEvent("RenderPauseText");
	}

	if (_isGameOver)
	{
		_events->EmitEvent("RenderGameOverText");
	}

	if (_isGameWon)
	{
		_events->EmitEvent("RenderGameWonText");
	}
}

void StateManager::Reset()
{
	_isPause = false;
	_isGameOver = false;
	_isGameWon = false;
}
