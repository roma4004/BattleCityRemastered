#pragma once

#include "RespawnManager.h"
#include "components/EventSystem.h"
#include <memory>
#include <vector>

class EventSystem;

struct PauseStatusEvent;
struct PreDrawUserInterfaceEvent;
struct GameResetEvent;
struct PlayersTeamIsWonEvent;
struct EnemiesTeamIsWonEvent;

class GameStateManager
{

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	bool _isPause{};
	bool _isGameOver{};
	bool _isGameWon{};

	void Subscribe();
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnPlayersTeamIsWon(const PlayersTeamIsWonEvent&);
	void OnEnemiesTeamIsWon(const EnemiesTeamIsWonEvent&);

	void Draw(const PreDrawUserInterfaceEvent&) const;
	void Reset(const GameResetEvent&);

public:
	explicit GameStateManager(const std::shared_ptr<EventSystem>& events);

	~GameStateManager() = default;
};
