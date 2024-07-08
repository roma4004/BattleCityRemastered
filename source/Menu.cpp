#include "../headers/Menu.h"
#include "../headers/GameMode.h"
#include "../headers/Point.h"
#include "../headers/utils/PixelUtils.h"

Menu::Menu(SDL_Renderer* renderer, TTF_Font* menuFont, std::shared_ptr<GameStatistics> statistics,
           const UPoint windowSize, int* windowBuffer, std::unique_ptr<InputProviderForMenu>& input,
           std::shared_ptr<EventSystem> events)
	: _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  yOffsetStart{static_cast<unsigned int>(_windowSize.y)},
	  _input{std::move(input)},
	  _events{std::move(events)},
	  _name{std::string("Statistics")},
	  _renderer{renderer},
	  _menuFont{menuFont},
	  _statistics{std::move(statistics)}
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
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_currentMode = newGameMode;
	});
	_events->AddListener("DrawMenuText", _name, [this]()
	{
		const auto menuKeysStats = _input->GetKeysStats();
		if (menuKeysStats.menuShow)
		{
			this->HandleMenuText(_renderer, _pos);
		}
	});

	_events->AddListener<const int>("EnemyRespawnResourceChangedTo", _name, [this](const int enemyRespawnResource)
	{
		this->_enemyRespawnResource = enemyRespawnResource;
	});
	_events->AddListener<const int>("PlayerOneRespawnResourceChangedTo", _name, [this](const int playerOneRespawnResource)
	{
		this->_playerOneRespawnResource = playerOneRespawnResource;
	});
	_events->AddListener<const int>("PlayerTwoRespawnResourceChangedTo", _name, [this](const int playerTwoRespawnResource)
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
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("DrawMenuText", _name);

	_events->RemoveListener<const int>("EnemyRespawnResourceChangedTo", _name);
	_events->RemoveListener<const int>("PlayerOneRespawnResourceChangedTo", _name);
	_events->RemoveListener<const int>("PlayerTwoRespawnResourceChangedTo", _name);
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
	const auto menuKeysStats = GetKeysStats();
	if (!menuKeysStats.menuShow)
	{
		return;
	}

	const unsigned int sizeX = static_cast<unsigned int>(_windowSize.x);
	const unsigned menuHeight = static_cast<unsigned int>(_windowSize.y) - 50;
	const unsigned menuWidth = sizeX - 228;
	for (_pos.y = 50 + yOffsetStart; _pos.y < menuHeight + yOffsetStart; ++_pos.y)
	{
		for (_pos.x = 50; _pos.x < menuWidth; ++_pos.x)
		{
			if (_pos.y < _windowSize.y && _pos.x < _windowSize.x)
			{
				constexpr unsigned int menuColor = 0xFF808080;
				int& targetColor = _windowBuffer[_pos.y * sizeX + _pos.x];
				unsigned int targetColorLessAlpha = PixelUtils::ChangeAlpha(static_cast<unsigned int>(targetColor), 91);
				targetColor = static_cast<int>(PixelUtils::BlendPixel(targetColorLessAlpha, menuColor));
			}
		}
	}

	// animation
	if (constexpr unsigned int yOffsetEnd = 0; yOffsetStart > yOffsetEnd)
	{
		yOffsetStart -= 3;
	}
}

void Menu::TextToRender(SDL_Renderer* renderer, const Point& pos, const SDL_Color& color, const int value) const
{
	TextToRender(renderer, pos, color, std::to_string(value));
}

void Menu::TextToRender(SDL_Renderer* renderer, const Point pos, const SDL_Color color,
                        const std::string& text) const
{
	SDL_Surface* surface = TTF_RenderText_Solid(_menuFont, text.c_str(), color);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, surface);

	const SDL_Rect textRect{pos.x, pos.y, surface->w, surface->h};
	SDL_RenderCopy(renderer, texture, nullptr, &textRect);

	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

void Menu::RenderStatistics(SDL_Renderer* renderer, const Point pos) const
{
	constexpr SDL_Color color = {0x00, 0xff, 0xff, 0xff};

	TextToRender(renderer, {pos.x - 60, pos.y + 100}, color, "GAME STATISTICS");
	TextToRender(renderer, {pos.x + 130, pos.y + 140}, color, "P1     P2     ENEMY");
	TextToRender(renderer, {pos.x - 130, pos.y + 160}, color, "RESPAWN REMAIN");
	TextToRender(renderer, {pos.x + 130, pos.y + 160}, color, _playerOneRespawnResource);
	TextToRender(renderer, {pos.x + 180, pos.y + 160}, color, _playerTwoRespawnResource);
	TextToRender(renderer, {pos.x + 235, pos.y + 160}, color, _enemyRespawnResource);

	TextToRender(renderer, {pos.x - 130, pos.y + 180}, color, "BULLET HIT BY BULLET");
	TextToRender(renderer, {pos.x + 130, pos.y + 180}, color, _statistics->GetBulletHitByPlayerOne());
	TextToRender(renderer, {pos.x + 180, pos.y + 180}, color, _statistics->GetBulletHitByPlayerTwo());
	TextToRender(renderer, {pos.x + 235, pos.y + 180}, color, _statistics->GetBulletHitByEnemy());

	TextToRender(renderer, {pos.x - 130, pos.y + 200}, color, "PLAYER HIT BY ENEMY");
	TextToRender(renderer, {pos.x + 130, pos.y + 200}, color, _statistics->GetPlayerOneHitByEnemyTeam());
	TextToRender(renderer, {pos.x + 180, pos.y + 200}, color, _statistics->GetPlayerTwoHitByEnemyTeam());

	TextToRender(renderer, {pos.x - 130, pos.y + 220}, color, "TANK KILLS");
	TextToRender(renderer, {pos.x + 130, pos.y + 220}, color, _statistics->GetEnemyDiedByPlayerOne());
	TextToRender(renderer, {pos.x + 180, pos.y + 220}, color, _statistics->GetEnemyDiedByPlayerTwo());
	TextToRender(renderer, {pos.x + 235, pos.y + 220}, color, _statistics->GetPlayerDiedByEnemyTeam());

	TextToRender(renderer, {pos.x - 130, pos.y + 240}, color, "ENEMY HIT BY");
	TextToRender(renderer, {pos.x + 130, pos.y + 240}, color, _statistics->GetEnemyHitByPlayerOne());
	TextToRender(renderer, {pos.x + 180, pos.y + 240}, color, _statistics->GetEnemyHitByPlayerTwo());
	TextToRender(renderer, {pos.x + 235, pos.y + 240}, color, _statistics->GetEnemyHitByFriendlyFire());

	TextToRender(renderer, {pos.x - 130, pos.y + 260}, color, "FRIEND HIT FRIEND");
	TextToRender(renderer, {pos.x + 130, pos.y + 260}, color, _statistics->GetPlayerOneHitFriendlyFire());
	TextToRender(renderer, {pos.x + 180, pos.y + 260}, color, _statistics->GetPlayerTwoHitFriendlyFire());
	TextToRender(renderer, {pos.x + 235, pos.y + 260}, color, _statistics->GetEnemyHitByFriendlyFire());

	TextToRender(renderer, {pos.x - 130, pos.y + 280}, color, "FRIEND KILLS FRIEND");
	TextToRender(renderer, {pos.x + 130, pos.y + 280}, color, _statistics->GetPlayerOneDiedByFriendlyFire());
	TextToRender(renderer, {pos.x + 180, pos.y + 280}, color, _statistics->GetPlayerTwoDiedByFriendlyFire());
	TextToRender(renderer, {pos.x + 235, pos.y + 280}, color, _statistics->GetEnemyDiedByFriendlyFire());

	TextToRender(renderer, {pos.x - 130, pos.y + 300}, color, "BRICK KILLS");
	TextToRender(renderer, {pos.x + 130, pos.y + 300}, color, _statistics->GetBrickDiedByPlayerOne());
	TextToRender(renderer, {pos.x + 180, pos.y + 300}, color, _statistics->GetBrickDiedByPlayerTwo());
	TextToRender(renderer, {pos.x + 235, pos.y + 300}, color, _statistics->GetBrickDiedByEnemyTeam());
}

void Menu::HandleMenuText(SDL_Renderer* renderer, const UPoint menuBackgroundPos) const
{
	const Point pos = {static_cast<int>(menuBackgroundPos.x - 350), static_cast<int>(menuBackgroundPos.y - 350)};
	constexpr SDL_Color color = {0xff, 0xff, 0xff, 0xff};

	TextToRender(renderer, {pos.x - 70, pos.y - 100}, color, "BATTLE CITY REMASTERED");
	TextToRender(renderer, {pos.x, pos.y - 40}, color, _currentMode == OnePlayer ? ">ONE PLAYER" : "ONE PLAYER");
	TextToRender(renderer, {pos.x, pos.y}, color, _currentMode == TwoPlayers ? ">TWO PLAYER" : "TWO PLAYER");
	TextToRender(renderer, {pos.x, pos.y + 40}, color, _currentMode == CoopWithAI ? ">COOP AI" : "COOP AI");

	RenderStatistics(renderer, pos);
}
