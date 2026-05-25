#include "components/Menu.h"
#include "application/UserInput.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "enums/GameMode.h"
#include <iomanip>
#include <sstream>

Menu::Menu(const UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _yOffsetStart{static_cast<unsigned int>(windowSize.y)}
	, _events{events}
	, _statistics{std::make_unique<GameStatistics>(events)}
	, _input{std::make_unique<InputProviderForMenu>(events)}
	, _name{std::string("Menu")}
	, _selectedGameMode{GameMode::OnePlayer}
{
	Subscribe();

	_padding = 25;
	_windowHeight = static_cast<int>(windowSize.y);
}

Menu::~Menu()
{
	Unsubscribe();
}

void Menu::Subscribe()
{
	_events->AddListener("SelectedGameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_selectedGameMode = newGameMode;
	});

	_events->AddListener("RespawnCountChangedTo", _name, [this](const std::string& objectName, const int respawnCount)
	{
		this->OnRespawnCountChanged(objectName, respawnCount);
	});

	if (_isMenuDisplayed)
	{
		_events->AddListener("DrawUserInterface", _name, [this]() { this->DrawMenu(); });
	}

	_events->AddListener("ShowMenu", _name, [this](const bool isMenuDisplayed)
	{
		_isMenuDisplayed = isMenuDisplayed;
		
		if (_isMenuDisplayed)
		{
			_events->AddListener("DrawUserInterface", _name, [this]() { this->DrawMenu(); });
		}
		else
		{
			_events->RemoveListener("DrawUserInterface", _name);
		}
	});
}

void Menu::Unsubscribe() const
{
	_events->RemoveListener("SelectedGameModeChangedTo", _name);
	_events->RemoveListener("RespawnCountChangedTo", _name);	

	if (_isMenuDisplayed)
	{
		_events->RemoveListener("DrawUserInterface", _name);
	}

	_events->RemoveListener("ShowMenu", _name);
}

//TODO: optimize draw call with cache non changed text part
void Menu::DrawMenu()
{
	// first time animation, slow scrolling from bottom corner to vertical center
	if (constexpr unsigned int yOffsetEnd = 0u; _yOffsetStart > yOffsetEnd)
	{
		_yOffsetStart -= 3;
	}

	_pos.x = _padding;
	_pos.y = static_cast<int>(_padding + _yOffsetStart);

	_events->EmitEvent("RenderMenuBackground", _pos);
	_events->EmitEvent("RenderMenuLogo", _pos);
	DrawText();
}

void Menu::RenderStatistics(const Point pos) const
{
	constexpr unsigned int color = {0xff00ffff};

	_events->EmitEvent("RenderText", Point{.x = pos.x - 60, .y = pos.y + 100}, color, "GAME STATISTICS");

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

	//TODO: display statistics for pickuped bonuses
	// RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 340}, color, "BONUS PICKUPS",
	//                         _statistics->GetBonusPickupByPlayerOne(),
	//                         _statistics->GetBonusPickupByPlayerTwo(),
	//                         _statistics->GetBonusPickupByEnemyTeam());
}

void Menu::RenderTextWithAlignment(const Point pos, const unsigned int color, const std::string& text,
								   const int player1, const int player2, const int enemy) const
{
	std::ostringstream textStream;
	textStream << std::left
			<< std::setw(22) << text
			<< std::setw(4) << player1
			<< std::setw(4) << player2;

	if (enemy != -1)
	{
		textStream << std::setw(4) << enemy;
	}

	_events->EmitEvent("RenderText", pos, color, textStream.str());
}

void Menu::RenderTextWithAlignment(const Point pos, const unsigned int color, const std::string& text,
								   const std::string& text2, const std::string& text3) const
{
	std::ostringstream textStream;

	textStream << std::left << std::setw(22) << std::setw(4) << text << std::setw(4) << text2 << std::setw(4) << text3;

	_events->EmitEvent("RenderText", Point{.x = pos.x, .y = pos.y}, color, textStream.str());
}

void Menu::DrawTextLine(Point& posText, bool isSelected, std::string text) const
{
	if (isSelected)
	{
		_events->EmitEvent("RenderMenuJoyIcon", Point{.x = posText.x - 35, .y = posText.y - 10});
	}

	constexpr unsigned int color = {0xffffffff};
	_events->EmitEvent("RenderText", posText, color, text);
	posText.y += 25;
}

void Menu::DrawText() const
{
	Point relativePosText{.x = _pos.x + 180, .y = _pos.y + 140};
	const Point posStatistics{.x = _pos.x + 180, .y = _pos.y + 170};
	if (relativePosText.y >= _windowHeight)
	{
		return;
	}

	DrawTextLine(relativePosText, _selectedGameMode == GameMode::OnePlayer, "ONE PLAYER");
	DrawTextLine(relativePosText, _selectedGameMode == GameMode::TwoPlayers, "TWO PLAYER");
	DrawTextLine(relativePosText, _selectedGameMode == GameMode::CoopWithBot, "COOP WITH BOT");
	DrawTextLine(relativePosText, _selectedGameMode == GameMode::PlayAsHost, "PLAY AS HOST"); 
	DrawTextLine(relativePosText, _selectedGameMode == GameMode::PlayAsClient, "PLAY AS CLIENT");

	RenderStatistics(posStatistics);
}

void Menu::OnRespawnCountChanged(const std::string& objectName, const int respawnCount)
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
