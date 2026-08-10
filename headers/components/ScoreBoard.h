#pragma once

#include "Point.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"

#include <memory>
#include <vector>

class EventSystem;
class GameStatistics;
struct GameResetEvent;
struct GameModeChangedToEvent;
struct RespawnCountChangedToEvent;
struct DrawUserInterfaceEvent;
struct MenuShowedEvent;
struct PauseStatusEvent;
struct PlayersTeamIsWonEvent;
struct EnemiesTeamIsWonEvent;

class ScoreBoard final
{
	Point _pos{};
	int _windowHeight{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<GameStatistics> _statistics{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime by DisplayScore() (also the initial Subscribe() call, if the scoreboard
	// starts shown), independent of _subs's fixed subscribe-once-at-construction lifetime.
	EventSubscription _drawSub{};

	std::string _name{};
	bool _isScoreBoardDisplayed{false};

	unsigned short _enemyRespawnCount{20u};
	unsigned short _playerOneRepawnCount{3u};
	unsigned short _playerTwoRespawnCount{3u};
	GameMode _gameMode{};

	void Subscribe();

	void OnGameReset(const GameResetEvent&);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&);
	void OnMenuShowed(const MenuShowedEvent& event);
	void OnPauseStatus(const PauseStatusEvent&);
	void OnPlayersTeamIsWon(const PlayersTeamIsWonEvent&);
	void OnEnemiesTeamIsWon(const EnemiesTeamIsWonEvent&);

	void RenderStatistics() const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, unsigned short player1,
								 unsigned short player2, unsigned short enemy) const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, unsigned short player1,
								 unsigned short player2) const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, const std::string& text2,
								 const std::string& text3) const;

	void DisplayScore(bool isDisplayed);

public:
	ScoreBoard(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	//NOTE: defaulted out-of-line in the .cpp (not here) - this header only forward-declares
	//GameStatistics, held below by unique_ptr, so an in-header default would need it complete here.
	~ScoreBoard();

	void Draw();
};
