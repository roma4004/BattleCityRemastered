#include "components/ScoreBoard.h"
#include "application/UserInput.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "enums/GameMode.h"
#include <iomanip>
#include <sstream>

ScoreBoard::ScoreBoard(const UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _events{events}
	, _statistics{std::make_unique<GameStatistics>(events)}
	, _name{std::string("ScoreBoard")}
{
	Subscribe();

	_padding = 25;
	_pos.x = _padding;
	_pos.y = _padding;
	_windowHeight = static_cast<int>(windowSize.y);
}

ScoreBoard::~ScoreBoard()
{
	Unsubscribe();
}

void ScoreBoard::Subscribe()
{
	//NOTE: avoid showing score on game start
	_events->AddListener("Reset", _name, [this]() { this->DisplayScore(false); });

	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
	});

	_events->AddListener(
			"RespawnCountChangedTo", _name,
			[this](const std::string& objectName, const unsigned short respawnCount)
			{
				this->OnRespawnCountChanged(objectName, respawnCount);//TODO: extract from score to sidebar
			});

	if (_isScoreBoardDisplayed)
	{
		_events->AddListener("DrawUserInterface", _name, [this]() { this->Draw(); });
	}

	//NOTE: avoid showing score and menu at the same time
	_events->AddListener("MenuShowed", _name, [this](const bool isDisplayed)
	{
		if (isDisplayed)
		{
			this->DisplayScore(false);
		}
	});
	_events->AddListener("Pause_Status", _name, [this](const bool isPause) { /*this->DisplayScore(isPause);*/ });
	_events->AddListener("PlayersTeamIsWon", _name, [this]() { this->DisplayScore(true); });
	_events->AddListener("EnemiesTeamIsWon", _name, [this]() { this->DisplayScore(true); });
}

void ScoreBoard::Unsubscribe() const { _events->RemoveAllListeners(_name); }

//TODO: optimize draw call with cache non changed text part
void ScoreBoard::Draw()
{
	_events->EmitEvent("RenderMenuBackground", _pos);
	RenderStatistics();
}

void ScoreBoard::RenderStatistics() const
{
	const Point pos{.x = _pos.x + 180, .y = _pos.y + 120};
	constexpr unsigned int color = {0xff00ffff};

	_events->EmitEvent("RenderText", Point{.x = pos.x - 60, .y = pos.y + 80}, color, "PRESS M TO SHOW MENU");
	_events->EmitEvent("RenderText", Point{.x = pos.x - 20, .y = pos.y + 120}, color, "GAME STATISTICS:");

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

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 220}, color, "TANK KILLS",
							_statistics->GetEnemyDiedByPlayerOne(),
							_statistics->GetEnemyDiedByPlayerTwo(),
							_statistics->GetPlayerDiedByEnemyTeam());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 240}, color, "ENEMY HIT BY",
							_statistics->GetEnemyHitByPlayerOne(),
							_statistics->GetEnemyHitByPlayerTwo(),
							_statistics->GetEnemyHitByFriendlyFire());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 260}, color, "FRIEND HIT FRIEND",
							_statistics->GetPlayerOneHitFriendlyFire(),
							_statistics->GetPlayerTwoHitFriendlyFire(),
							_statistics->GetEnemyHitByFriendlyFire());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 280}, color, "FRIEND KILLS FRIEND",
							_statistics->GetPlayerOneDiedByFriendlyFire(),
							_statistics->GetPlayerTwoDiedByFriendlyFire(),
							_statistics->GetEnemyDiedByFriendlyFire());

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 300}, color, "BRICKS KILLS",
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

	_events->EmitEvent("RenderText", pos, color, textStream.str());
}

void ScoreBoard::RenderTextWithAlignment(const Point pos, const unsigned int color, const std::string& text,
										 const unsigned short player1, const unsigned short player2) const
{
	std::ostringstream textStream;
	textStream << std::left
			<< std::setw(22) << text
			<< std::setw(4) << player1
			<< std::setw(4) << player2;

	_events->EmitEvent("RenderText", pos, color, textStream.str());
}

void ScoreBoard::RenderTextWithAlignment(const Point pos, const unsigned int color, const std::string& text,
										 const std::string& text2, const std::string& text3) const
{
	std::ostringstream textStream;

	textStream << std::left << std::setw(22) << std::setw(4) << text << std::setw(4) << text2 << std::setw(4) << text3;

	_events->EmitEvent("RenderText", Point{.x = pos.x, .y = pos.y}, color, textStream.str());
}

void ScoreBoard::OnRespawnCountChanged(const std::string& objectName, const unsigned short respawnCount)
{
	if (objectName == "Enemy")
	{
		_enemyRespawnCount = respawnCount;
	}
	else if (objectName == "Player1")
	{
		_playerOneRepawnCount = respawnCount;
	}
	else if (objectName == "Player2")
	{
		_playerTwoRespawnCount = respawnCount;
	}
}

void ScoreBoard::DisplayScore(const bool isDisplayed)
{
	if (isDisplayed && _gameMode == GameMode::Demo)
	{
		return;
	}

	if (isDisplayed)
	{
		_events->EmitEvent("ShowMenu", false);
	}

	_isScoreBoardDisplayed = isDisplayed;

	if (_isScoreBoardDisplayed)
	{
		_events->AddListener("DrawUserInterface", _name, [this]() { this->Draw(); });
	}
	else
	{
		_events->RemoveListener("DrawUserInterface", _name);
	}

	_events->EmitEvent("ScoreBoardShowed", isDisplayed);
}
