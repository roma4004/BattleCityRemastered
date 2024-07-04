#include "../headers/GameSuccess.h"
#include "../headers/CoopAI.h"
#include "../headers/Enemy.h"
#include "../headers/InputProviderForPlayerOne.h"
#include "../headers/InputProviderForPlayerTwo.h"
#include "../headers/Map.h"
#include "../headers/Menu.h"
#include "../headers/PlayerOne.h"
#include "../headers/PlayerTwo.h"

#include <algorithm>
#include <iostream>

GameSuccess::GameSuccess(const UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen,
                         TTF_Font* fpsFont, std::shared_ptr<EventSystem> events,
                         std::unique_ptr<InputProviderForMenu>& menuInput, std::unique_ptr<GameStatistics>& statistics)
	: _windowSize{windowSize},
	  _statistics{std::move(statistics)},
	  _menu{windowSize, windowBuffer, menuInput},
	  _windowBuffer{windowBuffer},
	  _renderer{renderer},
	  _screen{screen},
	  _fpsFont{fpsFont},
	  _events{std::move(events)},
	  _bulletPool{std::make_shared<BulletPool>()}
{
	ResetBattlefield();
}

GameSuccess::~GameSuccess() = default;

void GameSuccess::SpawnEnemyTanks(const float gridOffset, const float speed, const int health, const float size)
{
	SpawnEnemy(1, gridOffset, speed, health, size);
	SpawnEnemy(2, gridOffset, speed, health, size);
	SpawnEnemy(3, gridOffset, speed, health, size);
	SpawnEnemy(4, gridOffset, speed, health, size);
}

void GameSuccess::ResetBattlefield()
{
	_allObjects.clear();
	_allObjects.reserve(1000);

	_statistics->Reset();

	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	constexpr float speed = 142;
	constexpr int health = 100;
	const float size = gridOffset * 3;

	SpawnPlayerTanks(gridOffset, speed, health, size);

	SpawnEnemyTanks(gridOffset, speed, health, size);

	//Map creation
	//Map::ObstacleCreation<Brick>(&env, 30,30);
	//Map::ObstacleCreation<Iron>(&env, 310,310);
	const Map field{};
	field.MapCreation(&_allObjects, gridOffset, _windowBuffer, _windowSize, _events);
}

void GameSuccess::SetGameMode(const GameMode gameMode) { _currentMode = gameMode; }

void GameSuccess::PrevGameMode()
{
	int mode = _currentMode;
	--mode;

	constexpr int maxMode = static_cast<int>(EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode < minMode ? maxMode : mode;
	_currentMode = static_cast<GameMode>(newMode);
}

void GameSuccess::NextGameMode()
{
	int mode = _currentMode;
	++mode;

	constexpr int maxMode = static_cast<int>(EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode > maxMode ? minMode : mode;
	_currentMode = static_cast<GameMode>(newMode);
}

void GameSuccess::ClearBuffer() const
{
	const auto size = _windowSize.x * _windowSize.y * sizeof(int);
	memset(_windowBuffer, 0, size);
}

void GameSuccess::MouseEvents(const SDL_Event& event)
{
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
	{
		_mouseButtons.MouseLeftButton = true;
		std::cout << "MouseLeftButton: "
				<< "Down" << '\n';

		return;
	}
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
	{
		_mouseButtons.MouseLeftButton = false;
		std::cout << "MouseLeftButton: "
				<< "Up" << '\n';

		return;
	}

	if (event.type == SDL_MOUSEMOTION && _mouseButtons.MouseLeftButton)
	{
		const Sint32 x = event.motion.x;
		const Sint32 y = event.motion.y;
		std::cout << "x: " << x << " \t y: " << y << '\n';
		// const int rowSize = env.windowWidth; ???

		if (x < 1 || y < 1 ||
		    x >= static_cast<Sint32>(_windowSize.x) - 1 && y >= static_cast<Sint32>(_windowSize.y) - 1)
		{
			return;
		}
	}
}

void GameSuccess::KeyPressed(const SDL_Event& event) const
{
	switch (event.key.keysym.sym)
	{
		case SDLK_w:
			_events->EmitEvent("W_Pressed");
			break;
		case SDLK_a:
			_events->EmitEvent("A_Pressed");
			break;
		case SDLK_s:
			_events->EmitEvent("S_Pressed");
			break;
		case SDLK_d:
			_events->EmitEvent("D_Pressed");
			break;
		case SDLK_SPACE:
			_events->EmitEvent("Space_Pressed");
			break;
		case SDLK_UP:
			_events->EmitEvent("ArrowUp_Pressed");
			break;
		case SDLK_LEFT:
			_events->EmitEvent("ArrowLeft_Pressed");
			break;
		case SDLK_DOWN:
			_events->EmitEvent("ArrowDown_Pressed");
			break;
		case SDLK_RIGHT:
			_events->EmitEvent("ArrowRight_Pressed");
			break;
		case SDLK_RCTRL:
			_events->EmitEvent("RCTRL_Pressed");
			break;
		case SDLK_RETURN:
			_events->EmitEvent("Enter_Pressed");
			break;
		case SDLK_m:
			_events->EmitEvent("Menu_Pressed");
			break;
		case SDLK_p:
			_events->EmitEvent("Pause_Pressed");
			break;
		default:
			break;
	}
}

void GameSuccess::KeyReleased(const SDL_Event& event) const
{
	switch (event.key.keysym.sym)
	{
		case SDLK_w:
			_events->EmitEvent("W_Released");
			break;
		case SDLK_a:
			_events->EmitEvent("A_Released");
			break;
		case SDLK_s:
			_events->EmitEvent("S_Released");
			break;
		case SDLK_d:
			_events->EmitEvent("D_Released");
			break;
		case SDLK_SPACE:
			_events->EmitEvent("Space_Released");
			break;
		case SDLK_UP:
			_events->EmitEvent("ArrowUp_Released");
			break;
		case SDLK_LEFT:
			_events->EmitEvent("ArrowLeft_Released");
			break;
		case SDLK_DOWN:
			_events->EmitEvent("ArrowDown_Released");
			break;
		case SDLK_RIGHT:
			_events->EmitEvent("ArrowRight_Released");
			break;
		case SDLK_RCTRL:
			_events->EmitEvent("RCTRL_Released");
			break;
		case SDLK_RETURN:
			_events->EmitEvent("Enter_Released");
			break;
		case SDLK_m:
			_events->EmitEvent("Menu_Released");
			break;
		case SDLK_p:
			_events->EmitEvent("Pause_Released");
			break;
		default:
			break;
	}
}

void GameSuccess::KeyboardEvents(const SDL_Event& event) const
{
	if (event.type == SDL_KEYDOWN)
	{
		KeyPressed(event);
	}
	else if (event.type == SDL_KEYUP)
	{
		KeyReleased(event);
	}
}

void GameSuccess::TextToRender(SDL_Renderer* renderer, const Point& pos, const SDL_Color& color, const int value) const
{
	TextToRender(renderer, pos, color, std::to_string(value));
}

void GameSuccess::TextToRender(SDL_Renderer* renderer, const Point pos, const SDL_Color color,
                               const std::string& text) const
{
	SDL_Surface* surface = TTF_RenderText_Solid(_fpsFont, text.c_str(), color);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, surface);

	const SDL_Rect textRect{pos.x, pos.y, surface->w, surface->h};
	SDL_RenderCopy(renderer, texture, nullptr, &textRect);

	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

void GameSuccess::RenderStatistics(SDL_Renderer* renderer, const Point pos) const
{
	constexpr SDL_Color color = {0x00, 0xff, 0xff, 0xff};

	TextToRender(renderer, {pos.x - 60, pos.y + 100}, color, "GAME STATISTICS");
	TextToRender(renderer, {pos.x + 130, pos.y + 140}, color, "P1     P2     ENEMY");
	TextToRender(renderer, {pos.x - 130, pos.y + 160}, color, "RESPAWN REMAIN");
	TextToRender(renderer, {pos.x + 130, pos.y + 160}, color, _statistics->GetPlayerOneRespawnResource());
	TextToRender(renderer, {pos.x + 180, pos.y + 160}, color, _statistics->GetPlayerTwoRespawnResource());
	TextToRender(renderer, {pos.x + 235, pos.y + 160}, color, _statistics->GetEnemyRespawnResource());

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

void GameSuccess::HandleMenuText(SDL_Renderer* renderer, const UPoint menuBackgroundPos)
{
	if (_menu.input->keys.up)
	{
		PrevGameMode();
		_menu.input->keys.up = false;
	}
	else if (_menu.input->keys.down)
	{
		NextGameMode();
		_menu.input->keys.down = false;
	}
	else if (_menu.input->keys.reset)
	{
		ResetBattlefield();
		_menu.input->keys.reset = false;
		_menu.input->keys.menuShow = false;
	}

	//menu text
	const Point pos = {static_cast<int>(menuBackgroundPos.x - 350), static_cast<int>(menuBackgroundPos.y - 350)};
	constexpr SDL_Color color = {0xff, 0xff, 0xff, 0xff};

	TextToRender(renderer, {pos.x - 70, pos.y - 100}, color, "BATTLE CITY REMASTERED");
	TextToRender(renderer, {pos.x, pos.y - 40}, color, _currentMode == OnePlayer ? ">ONE PLAYER" : "ONE PLAYER");
	TextToRender(renderer, {pos.x, pos.y}, color, _currentMode == TwoPlayers ? ">TWO PLAYER" : "TWO PLAYER");
	TextToRender(renderer, {pos.x, pos.y + 40}, color, _currentMode == CoopWithAI ? ">COOP AI" : "COOP AI");

	RenderStatistics(renderer, pos);
}

void GameSuccess::HandleFPS(Uint32& frameCount, Uint64& fpsPrevUpdateTime, Uint32& fps, const Uint64 newTime)
{
	++frameCount;
	if (newTime - fpsPrevUpdateTime >= 1000)
	{
		if (fps != frameCount)
		{
			fps = frameCount;

			_fpsSurface = TTF_RenderText_Solid(_fpsFont, std::to_string(fps).c_str(), SDL_Color{140, 0, 255, 0});
			if (_fpsTexture)
			{
				SDL_DestroyTexture(_fpsTexture);
			}
			_fpsTexture = SDL_CreateTextureFromSurface(_renderer, _fpsSurface);

			SDL_FreeSurface(_fpsSurface);
		}
		fpsPrevUpdateTime = newTime;
		frameCount = 0;
	}
}

bool GameSuccess::IsCollideWith(const Rectangle& r1, const Rectangle& r2)
{
	// Check if one rectangle is to the right of the other
	if (r1.x > r2.x + r2.w || r2.x > r1.x + r1.w)
	{
		return false;
	}

	// Check if one rectangle is above the other
	if (r1.y > r2.y + r2.h || r2.y > r1.y + r1.h)
	{
		return false;
	}

	// If neither of the above conditions are met, the rectangles overlap
	return true;
}

void GameSuccess::SpawnPlayerTanks(const float gridOffset, const float speed, const int health, const float size)
{
	_menu.input->ToggleMenu();
	if (_currentMode == OnePlayer || _currentMode == TwoPlayers || _currentMode == CoopWithAI)
	{
		SpawnPlayer1(gridOffset, speed, health, size);
	}

	if (_currentMode == TwoPlayers)
	{
		SpawnPlayer2(gridOffset, speed, health, size);
	}

	if (_currentMode == Demo)
	{
		SpawnCoop1(gridOffset, speed, health, size);
		SpawnCoop2(gridOffset, speed, health, size);
	}

	if (_currentMode == CoopWithAI)
	{
		SpawnCoop2(gridOffset, speed, health, size);
	}
}

void GameSuccess::SpawnEnemy(const int index, const float gridOffset, const float speed, const int health,
                             const float size)
{
	std::vector<Rectangle> spawnPos{
			{gridOffset * 16.f - size * 2.f, 0, size, size},
			{gridOffset * 32.f - size * 2.f, 0, size, size},
			{gridOffset * 16.f + size * 2.f, 0, size, size},
			{gridOffset * 32.f + size * 2.f, 0, size, size}
	};
	for (auto& rect: spawnPos)
	{
		bool isFreeSpawnSpot = true;
		for (const std::shared_ptr<BaseObj>& pawn: _allObjects)
		{
			if (IsCollideWith(rect, pawn->GetShape()))
			{
				isFreeSpawnSpot = false;
			}
		}

		if (isFreeSpawnSpot)
		{
			constexpr int gray = 0x808080;
			const auto indexString = std::to_string(index);
			_allObjects.emplace_back(std::make_shared<Enemy>(rect, gray, health, _windowBuffer, _windowSize, DOWN,
			                                                 speed, &_allObjects, _events, "Enemy" + indexString,
			                                                 "EnemyTeam", _bulletPool));
			_events->EmitEvent("Enemy" + indexString + "_Spawn");
			return;
		}
	}
}

void GameSuccess::SpawnPlayer1(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 16.f, windowSizeY - size, size, size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& pawn: _allObjects)
	{
		if (IsCollideWith(rect, pawn->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int yellow = 0xeaea00;
		auto name = "PlayerOne";
		auto fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		_allObjects.emplace_back(std::make_shared<PlayerOne>(rect, yellow, health, _windowBuffer, _windowSize, UP,
		                                                     speed,
		                                                     &_allObjects, _events, name, fraction, inputProvider,
		                                                     _bulletPool));
	}
}

void GameSuccess::SpawnPlayer2(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 32.f, windowSizeY - size, size, size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& pawn: _allObjects)
	{
		if (IsCollideWith(rect, pawn->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int green = 0x408000;
		auto name = "PlayerTwo";
		auto fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerTwo>(name, _events);
		_allObjects.emplace_back(std::make_shared<PlayerTwo>(rect, green, health, _windowBuffer, _windowSize, UP, speed,
		                                                     &_allObjects, _events, name, fraction, inputProvider,
		                                                     _bulletPool));
	}
}

void GameSuccess::SpawnCoop1(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 16.f, windowSizeY - size, size, size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& pawn: _allObjects)
	{
		if (IsCollideWith(rect, pawn->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int yellow = 0xeaea00;
		auto name = "CoopOneAI";
		auto fraction = "PlayerTeam";
		_allObjects.emplace_back(std::make_shared<CoopAI>(rect, yellow, health, _windowBuffer, _windowSize, UP, speed,
		                                                  &_allObjects, _events, name, fraction, _bulletPool));
	}
}

void GameSuccess::SpawnCoop2(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 32.f, windowSizeY - size, size, size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& pawn: _allObjects)
	{
		if (IsCollideWith(rect, pawn->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int green = 0x408000;
		auto name = "CoopTwoAI";
		auto fraction = "PlayerTeam";
		_allObjects.emplace_back(std::make_shared<CoopAI>(rect, green, health, _windowBuffer, _windowSize, UP, speed,
		                                                  &_allObjects, _events, name, fraction, _bulletPool));
	}
}

void GameSuccess::RespawnTanks()
{
	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	const float size = gridOffset * 3;
	constexpr float speed = 142;
	constexpr int health = 100;
	if (_statistics->IsEnemyOneNeedRespawn() && _statistics->GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(1, gridOffset, speed, health, size);
	}
	if (_statistics->IsEnemyTwoNeedRespawn() && _statistics->GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(2, gridOffset, speed, health, size);
	}
	if (_statistics->IsEnemyThreeNeedRespawn() && _statistics->GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(3, gridOffset, speed, health, size);
	}
	if (_statistics->IsEnemyFourNeedRespawn() && _statistics->GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(4, gridOffset, speed, health, size);
	}
	if (_statistics->IsPlayerOneNeedRespawn() && _statistics->GetPlayerOneRespawnResource() > 0)
	{
		SpawnPlayer1(gridOffset, speed, health, size);
	}
	if (_statistics->IsPlayerTwoNeedRespawn() && _statistics->GetPlayerTwoRespawnResource() > 0)
	{
		SpawnPlayer2(gridOffset, speed, health, size);
	}
	if (_statistics->IsCoopOneAINeedRespawn() && _statistics->GetPlayerOneRespawnResource() > 0)
	{
		SpawnCoop1(gridOffset, speed, health, size);
	}
	if (_statistics->IsCoopTwoAINeedRespawn() && _statistics->GetPlayerTwoRespawnResource() > 0)
	{
		SpawnCoop2(gridOffset, speed, health, size);
	}
}

void GameSuccess::EventHandling()
{
	// event handling
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
		{
			_isGameOver = true;
		}

		MouseEvents(event);
		KeyboardEvents(event);
	}
}

void GameSuccess::MainLoop()
{
	Uint32 frameCount{0};
	Uint32 fps{0};
	float deltaTime{0.f};
	Uint64 oldTime = SDL_GetTicks64();
	auto fpsPrevUpdateTime = oldTime;
	const SDL_Rect fpsRectangle{
			/*x*/ static_cast<int>(_windowSize.x) - 80, /*y*/ 20, /*w*/ 40, /*h*/ 40};

	while (!_isGameOver)
	{
		// Cap to 60 FPS
		// SDL_Delay(static_cast<Uint32>(std::floor(16.666f - deltaTime)));

		ClearBuffer();

		EventHandling();

		if (!_menu.input->keys.pause)
		{
			_events->EmitEvent<float>("TickUpdate", deltaTime);
		}

		// TODO: separate bullets or somehow delete and recycle in one iterating through _allObjects (because now its two)
		// TODO: create wrapper for bullet and RAII to dispose it to bullet pool
		// Destroy all "dead" objects except bullet they will be recycled
		for (auto it = _allObjects.begin(); it < _allObjects.end();)
		{
			if ((*it)->GetIsAlive() == false)
			{
				if (const auto bullet = dynamic_cast<Bullet*>(it->get()); bullet != nullptr)
				{
					_bulletPool->ReturnBullet(*it);
					it = _allObjects.erase(it);
					continue;
				}
			}
			++it;
		}

		const auto it = std::ranges::remove_if(_allObjects, [&](const auto& obj) { return !obj->GetIsAlive(); }).
				begin();
		_allObjects.erase(it, _allObjects.end());

		RespawnTanks();

		_events->EmitEvent("Draw");
		_events->EmitEvent("DrawHealthBar");// TODO: create and blend separate buff layers(objects, effect, interface)

		const Uint64 newTime = SDL_GetTicks64();
		deltaTime = static_cast<float>(newTime - oldTime) / 1000.0f;
		HandleFPS(frameCount, fpsPrevUpdateTime, fps, newTime);

		if (_menu.input->keys.menuShow)
		{
			_menu.BlendToWindowBuffer();
		}

		// update screen with buffer
		SDL_UpdateTexture(_screen, nullptr, _windowBuffer, static_cast<int>(_windowSize.x) << 2);
		SDL_RenderCopy(_renderer, _screen, nullptr, nullptr);

		if (_menu.input->keys.menuShow)
		{
			HandleMenuText(_renderer, _menu._pos);
		}

		// Copy the texture with FPS to the renderer
		SDL_RenderCopy(_renderer, _fpsTexture, nullptr, &fpsRectangle);

		SDL_RenderPresent(_renderer);

		oldTime = newTime;
	}

	if (_fpsTexture)
	{
		SDL_DestroyTexture(_fpsTexture);
	}
}

int GameSuccess::Result() const { return 0; }
