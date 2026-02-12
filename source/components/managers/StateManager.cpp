#include "components/managers/StateManager.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"

StateManager::StateManager(const std::shared_ptr<EventSystem>& events)
	: _name{"StateManager"},
	  _events{events}
{
	Subscribe();
}

StateManager::~StateManager()
{
	Unsubscribe();
}

void StateManager::Subscribe()
{
	_events->AddListener("Pause_Status", _name, [this](const bool value) { this->_isPause = value; });
	// TODO: investigate why on demo after start we have skip one pause input
	_events->AddListener("SetGameOverText", _name, [this]() { this->_isGameOver = true; });
	_events->AddListener("PreDrawUserInterface", _name, [this]() { this->Draw(); });
	_events->AddListener("Reset", _name, [this]() { Reset(); });
	_events->AddListener("PlayerOneFinished", _name, [this]()
	{
		_playerOneLose = true;
		_isGameOver = IsGameOverReached();
	});
	_events->AddListener("PlayerTwoFinished", _name, [this]()
	{
		_playerTwoLose = true;
		_isGameOver = IsGameOverReached();
	});
	_events->AddListener("PlayersBaseFinished", _name, [this]()
	{
		_playersBaseLose = true;
		_isGameOver = IsGameOverReached();
	});
	_events->AddListener("PlayersTeamIsWon", _name, [this]()
	{
		_isGameWon = true;
		_isGameWon = IsGameWon();
	});
	_events->AddListener("EnemyDestroyed", _name, [this]() { _enemiesKillCount++; });
	_events->AddListener("EnemySpawned", _name, [this]() { _enemiesSpawnCount++; });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
	});
}

void StateManager::Unsubscribe() const
{
	_events->RemoveListener("Pause_Status", _name);
	_events->RemoveListener("SetGameOverText", _name);
	_events->RemoveListener("PreDrawUserInterface", _name);
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("PlayerOneFinished", _name);
	_events->RemoveListener("PlayerTwoFinished", _name);
	_events->RemoveListener("PlayersBaseFinished", _name);
	_events->RemoveListener("PlayersTeamIsWon", _name);
	_events->RemoveListener("EnemyDestroyed", _name);
	_events->RemoveListener("EnemySpawned", _name);
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
	_enemiesKillCount = 0;
	_enemiesSpawnCount = 0;
}

bool StateManager::IsGameOverReached() const
{
	return (_gameMode == GameMode::OnePlayer && _playerOneLose && _playersBaseLose)
	       || (_gameMode == GameMode::TwoPlayers && _playerOneLose && _playerTwoLose && _playersBaseLose)
	       || (_gameMode == GameMode::CoopWithBot && _playerOneLose && _playerTwoLose && _playersBaseLose)
	       || (_gameMode == GameMode::Demo && _playerOneLose && _playerTwoLose && _playersBaseLose);
}

bool StateManager::IsGameWon() const
{
	return (_gameMode == GameMode::OnePlayer && _enemiesSpawnCount == _enemiesKillCount)
	       || (_gameMode == GameMode::TwoPlayers && _enemiesSpawnCount == _enemiesKillCount)
	       || (_gameMode == GameMode::CoopWithBot && _enemiesSpawnCount == _enemiesKillCount);
}
