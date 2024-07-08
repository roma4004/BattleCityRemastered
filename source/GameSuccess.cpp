#include "../headers/GameSuccess.h"
#include "../headers/InputProviderForPlayerOne.h"
#include "../headers/InputProviderForPlayerTwo.h"
#include "../headers/Map.h"
#include "../headers/Menu.h"
#include "../headers/pawns/CoopAI.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PlayerOne.h"
#include "../headers/pawns/PlayerTwo.h"
#include "../headers/utils/ColliderCheck.h"

#include <algorithm>
#include <iostream>
#include <memory>

GameSuccess::GameSuccess(const UPoint windowSize, int* windowBuffer, SDL_Renderer* renderer, SDL_Texture* screen,
                         TTF_Font* fpsFont, const std::shared_ptr<EventSystem>& events,
                         std::unique_ptr<InputProviderForMenu>& menuInput,
                         const std::shared_ptr<GameStatistics>& statistics)
	: _windowSize{windowSize},
	  _statistics{statistics},
	  _menu{renderer, fpsFont, statistics, windowSize, windowBuffer, menuInput, events},
	  _windowBuffer{windowBuffer},
	  _renderer{renderer},
	  _screen{screen},
	  _fpsFont{fpsFont},
	  _events{events},
	  _bulletPool{std::make_shared<BulletPool>()},
	  _bonusSystem{events, &_allObjects, windowBuffer, windowSize}
{
	ResetBattlefield();
	NextGameMode();
	Subscribe();
}

GameSuccess::~GameSuccess()
{
	Unsubscribe();
}

void GameSuccess::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("PreviousGameMode", _name, [this]() { this->PrevGameMode(); });
	_events->AddListener("NextGameMode", _name, [this]() { this->NextGameMode(); });
	_events->AddListener("ResetBattlefield", _name, [this]() { this->ResetBattlefield(); });
	_events->AddListener<bool>("Pause_Status", _name, [this](const bool newPauseStatus)
	{
		this->_isPause = newPauseStatus;
	});

}

void GameSuccess::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("PreviousGameMode", _name);
	_events->RemoveListener("NextGameMode", _name);
	_events->RemoveListener("ResetBattlefield", _name);
	_events->RemoveListener<bool>("Pause_Status", _name);
}

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
	_events->EmitEvent<const GameMode>("GameModeChangedTo", _currentMode);
}

void GameSuccess::NextGameMode()
{
	int mode = _currentMode;
	++mode;

	constexpr int maxMode = static_cast<int>(EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode > maxMode ? minMode : mode;
	_currentMode = static_cast<GameMode>(newMode);
	_events->EmitEvent<const GameMode>("GameModeChangedTo", _currentMode);
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

void GameSuccess::SpawnPlayerTanks(const float gridOffset, const float speed, const int health, const float size)
{
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
		for (const std::shared_ptr<BaseObj>& object: _allObjects)
		{
			if (ColliderCheck::IsCollide(rect, object->GetShape()))
			{
				isFreeSpawnSpot = false;
			}
		}

		if (isFreeSpawnSpot)
		{
			constexpr int gray = 0x808080;
			std::string name = "Enemy" + std::to_string(index);
			std::string fraction = "EnemyTeam";
			_allObjects.emplace_back(std::make_shared<Enemy>(rect, gray, health, _windowBuffer, _windowSize, DOWN,
			                                                 speed, &_allObjects, _events, name, fraction,
			                                                 _bulletPool));
			return;
		}
	}
}

void GameSuccess::SpawnPlayer1(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 16.f, windowSizeY - size, size, size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& object: _allObjects)
	{
		if (ColliderCheck::IsCollide(rect, object->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int yellow = 0xeaea00;
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		_allObjects.emplace_back(std::make_shared<PlayerOne>(rect, yellow, health, _windowBuffer, _windowSize, UP,
		                                                     speed, &_allObjects, _events, name, fraction,
		                                                     inputProvider, _bulletPool));
	}
}

void GameSuccess::SpawnPlayer2(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 32.f, windowSizeY - size, size, size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& object: _allObjects)
	{
		if (ColliderCheck::IsCollide(rect, object->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int green = 0x408000;
		std::string name = "PlayerTwo";
		std::string fraction = "PlayerTeam";
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
	for (const std::shared_ptr<BaseObj>& object: _allObjects)
	{
		if (ColliderCheck::IsCollide(rect, object->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int yellow = 0xeaea00;
		std::string name = "CoopOneAI";
		std::string fraction = "PlayerTeam";
		_allObjects.emplace_back(std::make_shared<CoopAI>(rect, yellow, health, _windowBuffer, _windowSize, UP, speed,
		                                                  &_allObjects, _events, name, fraction, _bulletPool));
	}
}

void GameSuccess::SpawnCoop2(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 32.f, windowSizeY - size, size, size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& object: _allObjects)
	{
		if (ColliderCheck::IsCollide(rect, object->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int green = 0x408000;
		std::string name = "CoopTwoAI";
		std::string fraction = "PlayerTeam";
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

void GameSuccess::DisposeDeadObject()
{
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

		_menu.Update();

		if (!_isPause)
		{
			_events->EmitEvent<const float>("TickUpdate", deltaTime);
		}

		DisposeDeadObject();

		RespawnTanks();

		_events->EmitEvent("Draw");

		_events->EmitEvent("DrawHealthBar");// TODO: create and blend separate buff layers(objects, effect, interface)

		_events->EmitEvent("DrawMenuBackground");

		const Uint64 newTime = SDL_GetTicks64();
		deltaTime = static_cast<float>(newTime - oldTime) / 1000.0f;
		HandleFPS(frameCount, fpsPrevUpdateTime, fps, newTime);

		// update screen with buffer
		SDL_UpdateTexture(_screen, nullptr, _windowBuffer, static_cast<int>(_windowSize.x) << 2);
		SDL_RenderCopy(_renderer, _screen, nullptr, nullptr);

		_events->EmitEvent("DrawMenuText");

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
