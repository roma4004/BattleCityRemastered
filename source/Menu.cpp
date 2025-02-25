#include "../headers/Menu.h"
#include "../headers/EventSystem.h"
#include "../headers/GameStatistics.h"
#include "../headers/utils/PixelUtils.h"

Menu::Menu(std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<TTF_Font> menuFont,
           std::shared_ptr<SDL_Texture> menuLogo, std::shared_ptr<GameStatistics> statistics, UPoint windowSize,
           std::shared_ptr<int[]> windowBuffer, std::unique_ptr<InputProviderForMenu> input,
           std::shared_ptr<EventSystem> events)
	: _windowSize{std::move(windowSize)},
	  _yOffsetStart{static_cast<unsigned int>(_windowSize.y)},
	  _renderer{std::move(renderer)},
	  _windowBuffer{std::move(windowBuffer)},
	  _events{std::move(events)},
	  _menuFont{std::move(menuFont)},
	  _menuLogo{std::move(menuLogo)},
	  _statistics{std::move(statistics)},
	  _input{std::move(input)},
	  _name{std::string("Menu")}
{
	Subscribe();
}

Menu::~Menu()
{
	Unsubscribe();
}

void Menu::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("DrawMenuBackground", _name, [this]() { this->BlendBackgroundToWindowBuffer(); });

	_events->AddListener<const GameMode>("SelectedGameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_selectedGameMode = newGameMode;
	});

	_events->AddListener("DrawMenuText", _name, [this]()
	{
		if (const auto menuKeysStats = this->_input->GetKeysStats();
			menuKeysStats.menuShow)
		{
			this->HandleMenuText(_pos);
			const SDL_Rect logoRectangle{.x = static_cast<int>(this->_pos.x - 420),
			                             .y = static_cast<int>(this->_pos.y - 490),
			                             .w = 300,
			                             .h = 75
			};
			SDL_RenderCopy(this->_renderer.get(), this->_menuLogo.get(), nullptr, &logoRectangle);
		}
	});

	_events->AddListener<const int>(
			"EnemyRespawnResourceChangedTo", _name, [this](const int enemyRespawnResource)
			{
				this->_enemyRespawnResource = enemyRespawnResource;
			});

	_events->AddListener<const int>(
			"Player1RespawnResourceChangedTo", _name, [this](const int playerOneRespawnResource)
			{
				this->_playerOneRespawnResource = playerOneRespawnResource;
			});

	_events->AddListener<const int>(
			"Player2RespawnResourceChangedTo", _name, [this](const int playerTwoRespawnResource)
			{
				this->_playerTwoRespawnResource = playerTwoRespawnResource;
			});
}

void Menu::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("DrawMenuBackground", _name);
	_events->RemoveListener("SelectedGameModeChangedTo", _name);
	_events->RemoveListener("DrawMenuText", _name);

	_events->RemoveListener<const int>("EnemyRespawnResourceChangedTo", _name);
	_events->RemoveListener<const int>("Player1RespawnResourceChangedTo", _name);
	_events->RemoveListener<const int>("Player2RespawnResourceChangedTo", _name);
}

void Menu::Update() const
{
	const auto menuKeysStats = _input->GetKeysStats();

	if (menuKeysStats.up)
	{
		_events->EmitEvent("PreviousGameMode");
		_input->ToggleUp();
	}
	else if (menuKeysStats.down)
	{
		_events->EmitEvent("NextGameMode");
		_input->ToggleDown();
	}

	if (menuKeysStats.reset)
	{
		_events->EmitEvent("ResetBattlefield");
		_input->ToggleMenu();
	}
}

// blend menu panel and menu texture background
void Menu::BlendBackgroundToWindowBuffer()
{
	if (_windowBuffer == nullptr)
	{
		return;
	}

	if (const auto menuKeysStats = GetKeysStats(); !menuKeysStats.menuShow)
	{
		return;
	}

	const auto sizeX = static_cast<unsigned int>(_windowSize.x);
	const unsigned menuHeight = static_cast<unsigned int>(_windowSize.y) - 50;
	const unsigned menuWidth = sizeX - 228;
	for (_pos.y = 50 + _yOffsetStart; _pos.y < menuHeight + _yOffsetStart; ++_pos.y)
	{
		for (_pos.x = 50; _pos.x < menuWidth; ++_pos.x)
		{
			if (_pos.y < _windowSize.y && _pos.x < _windowSize.x)
			{
				constexpr unsigned int menuColor = 0xFF808080;
				int& targetColor = _windowBuffer.get()[_pos.y * sizeX + _pos.x];
				const unsigned int targetColorLessAlpha = PixelUtils::ChangeAlpha(
						static_cast<unsigned int>(targetColor), 91);
				targetColor = static_cast<int>(PixelUtils::BlendPixel(targetColorLessAlpha, menuColor));
			}
		}
	}

	// animation
	if (constexpr unsigned int yOffsetEnd = 0; _yOffsetStart > yOffsetEnd)
	{
		_yOffsetStart -= 3;
	}
}

void Menu::TextToRender(const Point& pos, const SDL_Color& color, const int value) const
{
	TextToRender(pos, color, std::to_string(value));
}

void Menu::TextToRender(const Point pos, const SDL_Color color, const std::string& text) const
{
	if (!_menuFont || !_renderer)
	{
		return;
	}

	const std::unique_ptr<SDL_Surface, void(*)(SDL_Surface*)> surface(
			TTF_RenderText_Solid(_menuFont.get(), text.c_str(), color),
			SDL_FreeSurface);
	if (!surface)
	{
		return;
	}

	const std::unique_ptr<SDL_Texture, void(*)(SDL_Texture*)> texture(
			SDL_CreateTextureFromSurface(_renderer.get(), surface.get()),
			SDL_DestroyTexture);
	if (!texture)
	{
		return;
	}

	const SDL_Rect textRect{pos.x, pos.y, surface->w, surface->h};
	SDL_RenderCopy(_renderer.get(), texture.get(), nullptr, &textRect);
}

void Menu::RenderStatistics(const Point pos) const
{
	constexpr SDL_Color color = {0x00, 0xff, 0xff, 0xff};

	TextToRender({.x = pos.x - 60, .y = pos.y + 100}, color, "GAME STATISTICS");
	TextToRender({.x = pos.x + 130, .y = pos.y + 140}, color, "P1     P2     ENEMY");
	TextToRender({.x = pos.x - 130, .y = pos.y + 160}, color, "RESPAWN REMAIN");
	TextToRender({.x = pos.x + 130, .y = pos.y + 160}, color, _playerOneRespawnResource);
	TextToRender({.x = pos.x + 180, .y = pos.y + 160}, color, _playerTwoRespawnResource);
	TextToRender({.x = pos.x + 235, .y = pos.y + 160}, color, _enemyRespawnResource);

	TextToRender({.x = pos.x - 130, .y = pos.y + 180}, color, "BULLET HIT BY BULLET");
	TextToRender({.x = pos.x + 130, .y = pos.y + 180}, color, _statistics->GetBulletHitByPlayerOne());
	TextToRender({.x = pos.x + 180, .y = pos.y + 180}, color, _statistics->GetBulletHitByPlayerTwo());
	TextToRender({.x = pos.x + 235, .y = pos.y + 180}, color, _statistics->GetBulletHitByEnemy());

	TextToRender({.x = pos.x - 130, .y = pos.y + 200}, color, "PLAYER HIT BY ENEMY");
	TextToRender({.x = pos.x + 130, .y = pos.y + 200}, color, _statistics->GetPlayerOneHitByEnemyTeam());
	TextToRender({.x = pos.x + 180, .y = pos.y + 200}, color, _statistics->GetPlayerTwoHitByEnemyTeam());

	TextToRender({.x = pos.x - 130, .y = pos.y + 220}, color, "TANK KILLS");
	TextToRender({.x = pos.x + 130, .y = pos.y + 220}, color, _statistics->GetEnemyDiedByPlayerOne());
	TextToRender({.x = pos.x + 180, .y = pos.y + 220}, color, _statistics->GetEnemyDiedByPlayerTwo());
	TextToRender({.x = pos.x + 235, .y = pos.y + 220}, color, _statistics->GetPlayerDiedByEnemyTeam());

	TextToRender({.x = pos.x - 130, .y = pos.y + 240}, color, "ENEMY HIT BY");
	TextToRender({.x = pos.x + 130, .y = pos.y + 240}, color, _statistics->GetEnemyHitByPlayerOne());
	TextToRender({.x = pos.x + 180, .y = pos.y + 240}, color, _statistics->GetEnemyHitByPlayerTwo());
	TextToRender({.x = pos.x + 235, .y = pos.y + 240}, color, _statistics->GetEnemyHitByFriendlyFire());

	TextToRender({.x = pos.x - 130, .y = pos.y + 260}, color, "FRIEND HIT FRIEND");
	TextToRender({.x = pos.x + 130, .y = pos.y + 260}, color, _statistics->GetPlayerOneHitFriendlyFire());
	TextToRender({.x = pos.x + 180, .y = pos.y + 260}, color, _statistics->GetPlayerTwoHitFriendlyFire());
	TextToRender({.x = pos.x + 235, .y = pos.y + 260}, color, _statistics->GetEnemyHitByFriendlyFire());

	TextToRender({.x = pos.x - 130, .y = pos.y + 280}, color, "FRIEND KILLS FRIEND");
	TextToRender({.x = pos.x + 130, .y = pos.y + 280}, color, _statistics->GetPlayerOneDiedByFriendlyFire());
	TextToRender({.x = pos.x + 180, .y = pos.y + 280}, color, _statistics->GetPlayerTwoDiedByFriendlyFire());
	TextToRender({.x = pos.x + 235, .y = pos.y + 280}, color, _statistics->GetEnemyDiedByFriendlyFire());

	TextToRender({.x = pos.x - 130, .y = pos.y + 300}, color, "BRICKWALL KILLS");
	TextToRender({.x = pos.x + 130, .y = pos.y + 300}, color, _statistics->GetBrickWallDiedByPlayerOne());
	TextToRender({.x = pos.x + 180, .y = pos.y + 300}, color, _statistics->GetBrickWallDiedByPlayerTwo());
	TextToRender({.x = pos.x + 235, .y = pos.y + 300}, color, _statistics->GetBrickWallDiedByEnemyTeam());
}

void Menu::HandleMenuText(const UPoint menuBackgroundPos) const
{
	const Point pos = {.x = static_cast<int>(menuBackgroundPos.x - 350),
	                   .y = static_cast<int>(menuBackgroundPos.y - 350)};
	constexpr SDL_Color color = {0xff, 0xff, 0xff, 0xff};

	TextToRender({.x = pos.x, .y = pos.y - 50}, color,
	             _selectedGameMode == OnePlayer ? ">ONE PLAYER" : "ONE PLAYER");
	TextToRender({.x = pos.x, .y = pos.y - 25}, color,
	             _selectedGameMode == TwoPlayers ? ">TWO PLAYER" : "TWO PLAYER");
	TextToRender({.x = pos.x, .y = pos.y}, color,
	             _selectedGameMode == CoopWithAI ? ">COOP AI" : "COOP AI");
	TextToRender({.x = pos.x, .y = pos.y + 25}, color,
	             _selectedGameMode == PlayAsHost ? ">PLAY AS HOST" : "PLAY AS HOST");
	TextToRender({.x = pos.x, .y = pos.y + 50}, color,
	             _selectedGameMode == PlayAsClient ? ">PLAY AS CLIENT" : "PLAY AS CLIENT");

	RenderStatistics(pos);
}
