#pragma once

#include <memory>
#include <string>

class EventSystem;
enum class GameMode : char8_t;

class StateManager
{
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};

	GameMode _gameMode{};
	int _enemiesSpawnCount{0};
	int _enemiesKillCount{0};
	bool _isPause{false};
	bool _isGameOver{false};
	bool _isGameWon{false};
	bool _playerOneLose{false};
	bool _playerTwoLose{false};
	bool _playersBaseLose{false};

	void Subscribe();
	void Unsubscribe() const;


	void Draw() const;

	void Reset();
	bool IsGameOverReached() const;
	bool IsGameWon() const;

public:
	StateManager(const std::shared_ptr<EventSystem>& events);

	~StateManager();
};
