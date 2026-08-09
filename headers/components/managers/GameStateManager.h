#pragma once

#include "RespawnManager.h"
#include "components/EventSystem.h"
#include <memory>
#include <string>
#include <vector>

class EventSystem;
class GameStateManager;

enum class GameMode : char8_t;

class GameStateManager
{
	std::string _name{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	GameMode _gameMode{};
	bool _isPause{};
	bool _isGameOver{};
	bool _isGameWon{};

	void Subscribe();

	void Draw() const;
	void Reset();

public:
	explicit GameStateManager(const std::shared_ptr<EventSystem>& events);

	~GameStateManager() = default;
};
