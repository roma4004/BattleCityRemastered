#include "../../headers/components/Menu.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/components/GameStatistics.h"
#include "../../headers/enums/GameMode.h"

Menu::Menu(std::shared_ptr<SDL_Renderer> renderer, std::shared_ptr<TTF_Font> menuFont,
           std::shared_ptr<SDL_Texture> menuLogo, std::shared_ptr<GameStatistics> statistics,
           std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events)
	: _yOffsetStart{static_cast<unsigned int>(window->size.y)},
	  _selectedGameMode{OnePlayer},
	  _window{std::move(window)},
	  _renderer{std::move(renderer)},
	  _events{events},
	  _menuFont{std::move(menuFont)},
	  _menuLogo{std::move(menuLogo)},
	  _statistics{std::move(statistics)},
	  _input{std::make_unique<InputProviderForMenu>(events)},
	  _name{std::string("Menu")}
{
	Subscribe();

	_padding = 25;
	const auto windowWidth = static_cast<unsigned int>(_window->size.x);
	_height = static_cast<int>(_window->size.y) - _padding * 3;
	constexpr int sideBarWidth = 228;
	_width = windowWidth - sideBarWidth - _padding;

	PregenerateMenuBackground();

	// SDL_SetRenderDrawBlendMode(_renderer.get(), SDL_BLENDMODE_BLEND);
	_backgroundTexture = std::shared_ptr<SDL_Texture>(
			SDL_CreateTexture(_renderer.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET,
			                  _width, _height),
			SDL_DestroyTexture);
	SDL_SetTextureBlendMode(_backgroundTexture.get(), SDL_BLENDMODE_BLEND);

	const SDL_Rect rect{_padding, _padding, _width, _height};
	SDL_UpdateTexture(_backgroundTexture.get(), &rect, _menuBackground.get(), _width << 2);
}

Menu::~Menu()
{
	Unsubscribe();
}

void Menu::Subscribe()
{
	_events->AddListener<const float>("TickUpdate", _name, [this](const float /*deltaTime*/)
	{
		this->TickUpdate();
	});

	_events->AddListener("DrawMenu", _name, [this]() { this->DrawMenu(); });

	_events->AddListener<const GameMode>("SelectedGameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_selectedGameMode = newGameMode;
	});

	_events->AddListener<const std::string&, const int>(
			"RespawnResourceChangedTo", _name, [this](const std::string& objectName, const int respawnResource)
			{
				this->OnRespawnResourceChanged(objectName, respawnResource);
			});
}

void Menu::Unsubscribe() const
{
	_events->RemoveListener<const float>("TickUpdate", _name);
	_events->RemoveListener("DrawMenu", _name);
	_events->RemoveListener("SelectedGameModeChangedTo", _name);
	_events->RemoveListener<const std::string&, const int>("RespawnResourceChangedTo", _name);
}

void Menu::TickUpdate() const
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

void Menu::PregenerateMenuBackground()
{
	_menuBackground = std::make_shared<int[]>(_height * _width);
	for (int y = 0; y < _height; ++y)
	{
		for (int x = 0; x < _width; ++x)
		{
			constexpr unsigned int menuColor = 0x91808080;// Alpha channel set to 0x80 for semi-transparency
			_menuBackground[y * _width + x] = menuColor;
		}
	}
}

void Menu::DrawMenu()
{
	if (const auto menuKeysStats = _input->GetKeysStats(); !menuKeysStats.menuShow)
	{
		return;
	}

	// animation
	if (constexpr unsigned int yOffsetEnd = 0u; _yOffsetStart > yOffsetEnd)
	{
		_yOffsetStart -= 3;
	}

	_pos.x = _padding;
	_pos.y = _padding + _yOffsetStart;

	DrawBackground();//TODO: sync animation speed background and text with logo
	DrawMenuLogo();
	DrawText();
}

// blend menu panel and menu texture background
void Menu::DrawBackground() const
{
	const SDL_Rect rect{_pos.x, _pos.y, _width, _height};

	SDL_UpdateTexture(_backgroundTexture.get(), &rect, _menuBackground.get(), _width << 2);
	SDL_RenderCopy(_renderer.get(), _backgroundTexture.get(), nullptr, &rect);
}

void Menu::DrawMenuLogo() const
{
	const SDL_Rect rect{.x = _pos.x + 135, .y = _pos.y + 42, .w = 300, .h = 75};

	SDL_RenderCopy(_renderer.get(), _menuLogo.get(), nullptr, &rect);
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

	//TODO: display statistics for pickuped bonuses
	// RenderTextWithAlignment({.x = pos.x - 130, .y = pos.y + 340}, color, "BONUS PICKUPS",
	//                         _statistics->GetBonusPickupByPlayerOne(),
	//                         _statistics->GetBonusPickupByPlayerTwo(),
	//                         _statistics->GetBonusPickupByEnemyTeam());
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

void Menu::DrawText() const
{
	//TODO: add skip if outside screen
	const Point pos = {.x = _pos.x + 180, .y = _pos.y + 180};
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

void Menu::OnRespawnResourceChanged(const std::string& objectName, const int respawnResource)
{
	if (objectName == "Enemy")
	{
		_enemyRespawnResource = respawnResource;
	}
	else if (objectName == "Player1")
	{
		_playerOneRespawnResource = respawnResource;
	}
	else if (objectName == "Player2")
	{
		_playerTwoRespawnResource = respawnResource;
	}
}
