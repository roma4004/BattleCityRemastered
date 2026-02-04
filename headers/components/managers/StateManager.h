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
	bool _isPause{false};
	bool _isGameOver{false};

	bool _playerOneFailState{false};
	bool _playerTwoFailState{false};
	bool _playersBaseFailState{false};

	void Subscribe();
	void Unsubscribe() const;

	void Draw() const;

	void Reset();
	bool IsGameOverReached() const;

public:
	StateManager(const std::shared_ptr<EventSystem>& events);

	~StateManager();
};
