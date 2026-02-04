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
	_events->AddListener("Pause_Status", _name, [this](const bool value) { this->_isPause = value; });// TODO: investigate why on demo after start we have skip one pause input
	_events->AddListener("SetGameOverText", _name, [this]() { this->_isGameOver = true; });
	_events->AddListener("PreDrawUserInterface", _name, [this]() { this->Draw(); });
	_events->AddListener("Reset", _name, [this]() { Reset(); });
	_events->AddListener("PlayerOneFinished", _name, [this]() { _playerOneFailState = true; _isGameOver = IsGameOverReached(); });
	_events->AddListener("PlayerTwoFinished", _name, [this]() { _playerTwoFailState = true; _isGameOver = IsGameOverReached(); }); 
	_events->AddListener("PlayersBaseFinished", _name, [this]() {_playersBaseFailState = true; _isGameOver = IsGameOverReached(); });
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
}

void StateManager::Reset()
{
	_isPause = false;
	_isGameOver = false;
}

bool StateManager::IsGameOverReached() const
{
	return (_gameMode == GameMode::OnePlayer && _playerOneFailState && _playersBaseFailState)
		|| (_gameMode == GameMode::TwoPlayers && _playerOneFailState && _playerTwoFailState && _playersBaseFailState)
		|| (_gameMode == GameMode::Demo && _playerOneFailState && _playerTwoFailState && _playersBaseFailState);
}
