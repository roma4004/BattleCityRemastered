#include "components/managers/StateManager.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include "enums/RespawnCount.h"

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
	_events->AddListener("PlayerDestroyed", _name, [this](){ _playersDeathCount++; });
	_events->AddListener("PlayerSpawned", _name, [this](){ _playersSpawnCount++; });
	_events->AddListener("PlayersBaseFinished", _name, [this]()
	{
		PlayersBaseFinished();
	});
	_events->AddListener("PlayersTeamIsWon", _name, [this]()
	{
		_isGameWon = true;
		_isGameWon = IsGameWon();
	});
	_events->AddListener("EnemiesTeamIsWon", _name, [this]()
	{
			_isGameOver = true;
			_isGameOver = IsGameOverReached();
	});
	_events->AddListener("EnemyDestroyed", _name, [this]() { _enemiesDeathCount++; });
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
	_events->RemoveListener("PlayerDestroyed", _name);
	_events->RemoveListener("PlayerSpawned", _name);
	_events->RemoveListener("PlayersBaseFinished", _name);
	_events->RemoveListener("PlayersTeamIsWon", _name);
	_events->RemoveListener("EnemiesTeamIsWon", _name);
	_events->RemoveListener("EnemyDestroyed", _name);
	_events->RemoveListener("EnemySpawned", _name);
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
	_playersBaseLose = false;
	_enemiesDeathCount = 0;
	_enemiesSpawnCount = 0;
	_playersDeathCount = 0;
	_playersSpawnCount = 0;
}

void StateManager::PlayersBaseFinished()
{
	_playersBaseLose = true;
	_isGameOver = IsGameOverReached();
}

bool StateManager::IsGameOverReached() const
{
	return (_gameMode == GameMode::OnePlayer && _isGameOver)
	       || (_gameMode == GameMode::OnePlayer && _isGameOver && _playersBaseLose)
	       || (_gameMode == GameMode::TwoPlayers && _isGameOver)
	       || (_gameMode == GameMode::TwoPlayers && _isGameOver && _playersBaseLose)
	       || (_gameMode == GameMode::CoopWithBot && _isGameOver)
	       || (_gameMode == GameMode::CoopWithBot && _isGameOver && _playersBaseLose)
	       || (_gameMode == GameMode::Demo && _isGameOver)
	       || (_gameMode == GameMode::Demo && _isGameOver && _playersBaseLose);       
}

bool StateManager::IsGameWon() const
{
	return (_gameMode == GameMode::OnePlayer && _enemiesSpawnCount == _enemiesDeathCount)
	       || (_gameMode == GameMode::TwoPlayers && _enemiesSpawnCount == _enemiesDeathCount)
	       || (_gameMode == GameMode::CoopWithBot && _enemiesSpawnCount == _enemiesDeathCount);
}
