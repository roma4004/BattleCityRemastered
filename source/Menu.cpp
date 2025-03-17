#include "../headers/Menu.h"
#include "../headers/EventSystem.h"
#include "../headers/GameStatistics.h"
#include "../headers/utils/PixelUtils.h"

Menu::Menu(std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<TTF_Font> menuFont,
           std::shared_ptr<SDL_Texture> menuLogo, std::shared_ptr<GameStatistics> statistics,
           std::shared_ptr<Window> window, std::unique_ptr<InputProviderForMenu> input,
           std::shared_ptr<EventSystem> events)
	: _yOffsetStart{static_cast<unsigned int>(window->size.y)},
	  _window{window},
	  _renderer{std::move(renderer)},
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
			this->DrawMenuText(_pos);
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

	_events->AddListener<const std::string&, const std::string&, const int>(
			"ClientReceived_RespawnResourceChanged", _name,
			[this](const std::string& author, const std::string& fraction, const int respawnResource)
			{
				this->OnRespawnResourceChanged(author, fraction, respawnResource);
			});
}

void Menu::Unsubscribe() const
{
	_events->RemoveListener("DrawMenuBackground", _name);
	_events->RemoveListener("SelectedGameModeChangedTo", _name);
	_events->RemoveListener("DrawMenuText", _name);

	_events->RemoveListener<const int>("EnemyRespawnResourceChangedTo", _name);
	_events->RemoveListener<const int>("Player1RespawnResourceChangedTo", _name);
	_events->RemoveListener<const int>("Player2RespawnResourceChangedTo", _name);
	_events->RemoveListener<const std::string&, const std::string&, const int>(
			"ClientReceived_RespawnResourceChanged", _name);
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
		_input->ToggleMenuInputSubscription();
	}
}

// blend menu panel and menu texture background
void Menu::BlendBackgroundToWindowBuffer()
{
	if (const auto menuKeysStats = GetKeysStats(); !menuKeysStats.menuShow)
	{
		return;
	}

	constexpr int padding = 50;
	const auto winSizeX = static_cast<unsigned int>(_window->size.x);
	const unsigned menuHeight = static_cast<unsigned int>(_window->size.y) - padding;
	const unsigned menuWidth = winSizeX - 228;
	const auto buffer = _window->buffer.get();
	for (_pos.y = padding + _yOffsetStart; _pos.y < menuHeight + _yOffsetStart; ++_pos.y)
	{
		for (_pos.x = padding; _pos.x < menuWidth; ++_pos.x)
		{
			if (_pos.y < _window->size.y)
			{
				constexpr unsigned int menuColor = 0xFF808080;
				int& targetColor = buffer[_pos.y * winSizeX + _pos.x];
				const unsigned int targetColorLessAlpha = PixelUtils::ChangeAlpha(
						static_cast<unsigned int>(targetColor), 91);
				targetColor = static_cast<int>(PixelUtils::BlendPixel(targetColorLessAlpha, menuColor));
				//TODO:blend using thread pool
			}
		}
	}

	// animation
	if (constexpr unsigned int yOffsetEnd = 0u; _yOffsetStart > yOffsetEnd)
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

	RenderTextWithAlignment({.x = pos.x + 180, .y = pos.y + 140}, color, "P1", "P2", "ENEMY");

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 160}, color, "RESPAWN REMAIN",
	                        _playerOneRespawnResource,
	                        _playerTwoRespawnResource,
	                        _enemyRespawnResource);

	RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 160}, color, "RESPAWN REMAIN",
	                        _playerOneRespawnResource,
	                        _playerTwoRespawnResource,
	                        _enemyRespawnResource);

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
}

void Menu::RenderTextWithAlignment(const Point pos, const SDL_Color color, const std::string& text, const int player1,
                                   const int player2, const int enemy) const
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

	TextToRender(Point{pos.x, pos.y}, color, textStream.str());
}

void Menu::RenderTextWithAlignment(const Point pos, const SDL_Color color, const std::string& text,
                                   const std::string& text2, const std::string& text3) const
{
	std::ostringstream textStream;

	textStream << std::left
			<< std::setw(22)
			<< std::setw(4) << text
			<< std::setw(4) << text2
			<< std::setw(4) << text3;

	TextToRender(Point{pos.x, pos.y}, color, textStream.str());
}

void Menu::DrawMenuText(const UPoint menuBackgroundPos) const
{
	const Point pos = {.x = static_cast<int>(menuBackgroundPos.x) - 375,
	                   .y = static_cast<int>(menuBackgroundPos.y) - 350};
	constexpr SDL_Color color = {0xff, 0xff, 0xff, 0xff};

	TextToRender({.x = pos.x, .y = pos.y - 50}, color,
	             _selectedGameMode == OnePlayer ? "->ONE PLAYER" : "ONE PLAYER");
	TextToRender({.x = pos.x, .y = pos.y - 25}, color,
	             _selectedGameMode == TwoPlayers ? "=>TWO PLAYER" : "TWO PLAYER");
	TextToRender({.x = pos.x, .y = pos.y}, color,
	             _selectedGameMode == CoopWithBot ? "->COOP WITH BOT" : "COOP WITH BOT");
	TextToRender({.x = pos.x, .y = pos.y + 25}, color,
	             _selectedGameMode == PlayAsHost ? "=>PLAY AS HOST" : "PLAY AS HOST");
	TextToRender({.x = pos.x, .y = pos.y + 50}, color,
	             _selectedGameMode == PlayAsClient ? "=>PLAY AS CLIENT" : "PLAY AS CLIENT");

	RenderStatistics(pos);
}

void Menu::OnRespawnResourceChanged(const std::string& author, const std::string& fraction, const int respawnResource)
{
	if (fraction == "EnemyTeam")
	{
		_enemyRespawnResource = respawnResource;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1")
		{
			_playerOneRespawnResource = respawnResource;
		}
		else if (author == "Player2")
		{
			_playerTwoRespawnResource = respawnResource;
		}
	}
}
