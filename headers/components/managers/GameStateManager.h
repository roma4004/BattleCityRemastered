#pragma once

#include "RespawnManager.h"
#include "components/EventSystem.h"
#include <memory>
#include <string>
#include <vector>

class EventSystem;
class GameStateManager;

enum class GameMode : char8_t;
struct PauseStatusEvent;
struct PreDrawUserInterfaceEvent;
struct GameResetEvent;
struct PlayersTeamIsWonEvent;
struct EnemiesTeamIsWonEvent;
struct GameModeChangedToEvent;

class GameStateManager
{

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	GameMode _gameMode{};
	bool _isPause{};
	bool _isGameOver{};
	bool _isGameWon{};

	void Subscribe();
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnPlayersTeamIsWon(const PlayersTeamIsWonEvent&);
	void OnEnemiesTeamIsWon(const EnemiesTeamIsWonEvent&);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);

	void Draw(const PreDrawUserInterfaceEvent&) const;
	void Reset(const GameResetEvent&);

public:
	explicit GameStateManager(const std::shared_ptr<EventSystem>& events);

	~GameStateManager() = default;
};
