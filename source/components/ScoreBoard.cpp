#include "components/ScoreBoard.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/events/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "enums/GameMode.h"
#include <iomanip>
#include <sstream>

ScoreBoard::ScoreBoard(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _pos{.x = 25, .y = 25}
	, _events{events}
	, _statistics{std::make_unique<GameStatistics>(events)}
	, _gameConfig{gameConfig}
{
	Subscribe();

	_windowHeight = static_cast<int>(gameConfig.windowSize.y);
}

ScoreBoard::~ScoreBoard() = default;

void ScoreBoard::Subscribe()
{
	//NOTE: avoid showing score on game start
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnGameReset));

	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnRespawnCountChangedTo));

	if (_isScoreBoardDisplayed)
	{
		_drawSub = _events->AddListener(this, &ScoreBoard::OnDrawUserInterface);
	}

	//NOTE: avoid showing score and menu at the same time
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnMenuShowed));
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnPlayersTeamIsWon));
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnEnemiesTeamIsWon));
}

void ScoreBoard::OnGameReset(const GameResetEvent&) { DisplayScore(false); }

void ScoreBoard::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	const auto& objectName = event.objectName;
	if (objectName == "Enemy")
	{
		_enemyRespawnCount = event.respawnCount;
	}
	else if (objectName == "Player1")
	{
		_playerOneRepawnCount = event.respawnCount;
	}
	else if (objectName == "Player2")
	{
		_playerTwoRespawnCount = event.respawnCount;
	}
}

void ScoreBoard::OnDrawUserInterface(const DrawUserInterfaceEvent&) { Draw(); }

void ScoreBoard::OnMenuShowed(const MenuShowedEvent& event)
{
	if (event.isShown)
	{
		DisplayScore(false);
	}
}

void ScoreBoard::OnPauseStatus(const PauseStatusEvent& /*event*/)
{
	/*DisplayScore(isPause);*/
}

void ScoreBoard::OnPlayersTeamIsWon(const PlayersTeamIsWonEvent&) { DisplayScore(true); }
void ScoreBoard::OnEnemiesTeamIsWon(const EnemiesTeamIsWonEvent&) { DisplayScore(true); }

//TODO: optimize draw call with cache non changed text part
void ScoreBoard::Draw() const
{
	_events->EmitEvent(RenderMenuBackgroundEvent{.pos = _pos});
	RenderStatistics();
}

void ScoreBoard::RenderStatistics() const
{
	const Point pos{.x = _pos.x + 180, .y = _pos.y + 120};
	constexpr unsigned int color = {0xff00ffffu};

	_events->EmitEvent(
			RenderTextEvent{.pos = Point{.x = pos.x - 60, .y = pos.y + 80},
							.color = color,
							.text = "PRESS M TO SHOW MENU"});
	_events->EmitEvent(
			RenderTextEvent{.pos = Point{.x = pos.x - 20, .y = pos.y + 120},
							.color = color,
							.text = "GAME STATISTICS:"});

	RenderTextWithAlignment({.x = pos.x + 180, .y = pos.y + 140}, color, "P1", "P2", "ENEMY");

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 160}, color, "RESPAWN REMAIN", _playerOneRepawnCount,
							_playerTwoRespawnCount,
							_enemyRespawnCount);

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 160}, color, "RESPAWN REMAIN", _playerOneRepawnCount,
							_playerTwoRespawnCount,
							_enemyRespawnCount);

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 180}, color, "BULLET HIT BY BULLET",
							_statistics->GetBulletHitByPlayerOne(),
							_statistics->GetBulletHitByPlayerTwo(),
							_statistics->GetBulletHitByEnemy());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 200}, color, "PLAYER HIT BY ENEMY",
							_statistics->GetPlayerOneHitByEnemyTeam(),
							_statistics->GetPlayerTwoHitByEnemyTeam());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 220}, color, "ENEMY HIT BY",
							_statistics->GetEnemyHitByPlayerOne(),
							_statistics->GetEnemyHitByPlayerTwo(),
							_statistics->GetEnemyHitByFriendlyFire());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 240}, color, "TANK KILLS",
							_statistics->GetEnemyDiedByPlayerOne(),
							_statistics->GetEnemyDiedByPlayerTwo(),
							_statistics->GetPlayerDiedByEnemyTeam());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 260}, color, "FRIENDLY HITS",
							_statistics->GetPlayerOneHitFriendlyFire(),
							_statistics->GetPlayerTwoHitFriendlyFire(),
							_statistics->GetEnemyHitByFriendlyFire());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 280}, color, "FRIENDLY KILLS",
							_statistics->GetPlayerOneDiedByFriendlyFire(),
							_statistics->GetPlayerTwoDiedByFriendlyFire(),
							_statistics->GetEnemyDiedByFriendlyFire());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 300}, color, "BRICK KILLS",
							_statistics->GetBrickWallDiedByPlayerOne(),
							_statistics->GetBrickWallDiedByPlayerTwo(),
							_statistics->GetBrickWallDiedByEnemyTeam());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 320}, color, "STEEL KILLS",
							_statistics->GetSteelWallDiedByPlayerOne(),
							_statistics->GetSteelWallDiedByPlayerTwo(),
							_statistics->GetSteelWallDiedByEnemyTeam());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 340}, color, "BONUS PICKUPS",
							_statistics->GetBonusPickupByPlayerOne(),
							_statistics->GetBonusPickupByPlayerTwo(),
							_statistics->GetBonusPickupByEnemyTeam());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 360}, color, "BONUS DESTROYED",
							_statistics->GetBonusDestroyedByPlayerOne(),
							_statistics->GetBonusDestroyedByPlayerTwo(),
							_statistics->GetBonusDestroyedByEnemyTeam());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 380}, color, "BONUS EXPIRED",
							_statistics->GetBonusExpired());
}

void ScoreBoard::RenderTextWithAlignment(const Point pos, const unsigned int color, const std::string& text,
										 const unsigned short player1, const unsigned short player2,
										 const unsigned short enemy) const
{
	std::ostringstream textStream;
	textStream << std::left
			<< std::setw(22) << text
			<< std::setw(4) << player1
			<< std::setw(4) << player2
			<< std::setw(4) << enemy;

	_events->EmitEvent(RenderTextEvent{.pos = pos, .color = color, .text = textStream.str()});
}

void ScoreBoard::RenderTextWithAlignment(const Point pos, const unsigned int color, const std::string& text,
										 const unsigned short player1, const unsigned short player2) const
{
	std::ostringstream textStream;
	textStream << std::left
			<< std::setw(22) << text
			<< std::setw(4) << player1
			<< std::setw(4) << player2;

	_events->EmitEvent(RenderTextEvent{.pos = pos, .color = color, .text = textStream.str()});
}

void ScoreBoard::RenderTextWithAlignment(const Point pos, const unsigned int color, const std::string& text,
										 const unsigned short total) const
{
	std::ostringstream textStream;
	textStream << std::left
			<< std::setw(22) << text
			<< std::setw(4) << total;

	_events->EmitEvent(RenderTextEvent{.pos = pos, .color = color, .text = textStream.str()});
}

void ScoreBoard::RenderTextWithAlignment(const Point pos, const unsigned int color, const std::string& text,
										 const std::string& text2, const std::string& text3) const
{
	std::ostringstream textStream;

	textStream << std::left << std::setw(22) << std::setw(4) << text << std::setw(4) << text2 << std::setw(4) << text3;

	_events->EmitEvent(RenderTextEvent{.pos = Point{.x = pos.x, .y = pos.y}, .color = color, .text = textStream.str()});
}

void ScoreBoard::DisplayScore(const bool isDisplayed)
{
	if (isDisplayed && _gameConfig.gameMode == GameMode::Demo)
	{
		return;
	}

	if (isDisplayed)
	{
		_events->EmitEvent(ShowMenuEvent{.show = false});
	}

	_isScoreBoardDisplayed = isDisplayed;

	if (_isScoreBoardDisplayed)
	{
		_drawSub = _events->AddListener(this, &ScoreBoard::OnDrawUserInterface);
	}
	else
	{
		_drawSub = EventSubscription{};
	}

	_events->EmitEvent(ScoreBoardShowedEvent{.isDisplayed = isDisplayed});
}
