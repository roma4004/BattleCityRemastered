#pragma once

#include "RespawnManager.h"
#include <memory>
#include <string>

class EventSystem;
class StateManager;

enum class GameMode : char8_t;

class StateManager
{
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};

	GameMode _gameMode{};
	bool _isPause{};
	bool _isGameOver{};
	bool _isGameWon{};

	void Subscribe();
	void Unsubscribe() const;

	void Draw() const;
	void Reset();

public:
	StateManager(const std::shared_ptr<EventSystem>& events);

	~StateManager();
};
