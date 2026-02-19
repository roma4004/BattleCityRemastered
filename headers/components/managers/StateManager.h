#pragma once

#include <memory>
#include <string>
#include "RespawnManager.h"

class EventSystem;
class StateManager;

enum class GameMode : char8_t;

class StateManager
{
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};

	GameMode _gameMode{};
	bool _isPause{false};
	bool _isGameOver{false};
	bool _isGameWon{false};

	void Subscribe();
	void Unsubscribe() const;

	void Draw() const;
	void Reset();

public:
	StateManager(const std::shared_ptr<EventSystem>& events);

	~StateManager();
};
