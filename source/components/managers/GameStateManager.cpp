#include "components/managers/GameStateManager.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"

GameStateManager::GameStateManager(const std::shared_ptr<EventSystem>& events)
	: _name{"GameStateManager"}
	, _events{events}
{
	Subscribe();
}

GameStateManager::~GameStateManager() { Unsubscribe(); }

void GameStateManager::Subscribe()
{
	_events->AddListener("Pause_Status", _name, [this](const bool isPause) { this->_isPause = isPause; });
	_events->AddListener("PreDrawUserInterface", _name, [this]() { this->Draw(); });
	_events->AddListener("Reset", _name, [this]() { this->Reset(); });
	_events->AddListener("PlayersTeamIsWon", _name, [this]() { this->_isGameWon = true; });
	_events->AddListener("EnemiesTeamIsWon", _name, [this]() { this->_isGameOver = true; });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
	});
}

void GameStateManager::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void GameStateManager::Draw() const
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

void GameStateManager::Reset()
{
	_isPause = false;
	_isGameOver = false;
	_isGameWon = false;
}
