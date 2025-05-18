#include "../../headers/application/GameSuccess.h"
#include "../../headers/Map.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/BulletPool.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/components/Menu.h"
#include "../../headers/components/TankSpawner.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/network/ClientHandler.h"
#include "../../headers/network/ServerHandler.h"

#include <algorithm>
//#include <fstream>
#include <iostream>
#include <memory>

//#ifdef _WIN32
//#define _WIN32_WINNT 0x0A00
//#endif
#define ASIO_STANDALONE

// std::ofstream error_log_server("error_log_Server.txt");
GameSuccess::GameSuccess(std::shared_ptr<Window> window, std::shared_ptr<SDL_Renderer> renderer,
                         std::shared_ptr<SDL_Texture> screen, std::shared_ptr<TTF_Font> fpsFont,
                         std::shared_ptr<EventSystem> events, std::shared_ptr<GameStatistics> statistics,
						 std::unique_ptr<Menu> menu, std::shared_ptr<SDL_Texture> atlasTexture,
                         const bool isVsyncOn)
	: _selectedGameMode{OnePlayer},
	  _menu{std::move(menu)},
	  _statistics{std::move(statistics)},
	  _window{window},
	  _renderer{std::move(renderer)},
	  _screen{std::move(screen)},
	  _fpsFont{std::move(fpsFont)},
	  _events{events},
	  _bulletPool{std::make_shared<BulletPool>(events, &_allObjects, window, Demo, atlasTexture, _renderer)},
	  _userInput{window, events},
	  _bonusSpawner{events, &_allObjects, window},
	  _obstacleSpawner{events, &_allObjects, window},
	  
	  _isVsyncOn{isVsyncOn}
{
	_tankSpawner = std::make_shared<TankSpawner>(window, &_allObjects, events, _bulletPool, atlasTexture, _renderer);

	GenerateFpsTextures();

	Subscribe();

	ResetBattlefield(Demo);
}

GameSuccess::~GameSuccess()
{
	Unsubscribe();
}

void GameSuccess::Subscribe()
{
	_events->AddListener("PreviousGameMode", _name, [this]() { this->PrevGameMode(); });
	_events->AddListener("NextGameMode", _name, [this]() { this->NextGameMode(); });
	_events->AddListener("ResetBattlefield", _name, [this]() { this->ResetBattlefield(this->_selectedGameMode); });

	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		if (_gameMode == PlayAsHost)
		{
			_networkNode = std::make_unique<ServerHandler>(_events);
		}
		else if (_gameMode == PlayAsClient)
		{
			_networkNode = std::make_unique<ClientHandler>(_events);
		}
	});
}

void GameSuccess::Unsubscribe() const
{
	_events->RemoveListener("PreviousGameMode", _name);
	_events->RemoveListener("NextGameMode", _name);
	_events->RemoveListener("ResetBattlefield", _name);

	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);
}

void GameSuccess::ResetBattlefield(const GameMode gameMode)
{
	_allObjects.clear();
	_allObjects.reserve(1000);

	SetCurrentGameMode(gameMode);
	if (gameMode == PlayAsClient || gameMode == PlayAsHost)
	{
		_events->EmitEvent("Pause_Released");
	}

	_events->EmitEvent("Reset");

	//Map creation
	const float gridOffset = static_cast<float>(_window->size.y) / 50.f;
	const Map field{&_obstacleSpawner};//TODO: replace with obstacleSpawner->mapLoad(map)
	field.MapCreation(gridOffset);
}

void GameSuccess::PrevGameMode()
{
	int mode = _selectedGameMode;
	--mode;

	constexpr int maxMode = static_cast<int>(EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode < minMode ? maxMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent<const GameMode>("SelectedGameModeChangedTo", _selectedGameMode);
}

void GameSuccess::NextGameMode()
{
	int mode = _selectedGameMode;
	++mode;

	constexpr int maxMode = static_cast<int>(EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode > maxMode ? minMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent<const GameMode>("SelectedGameModeChangedTo", _selectedGameMode);
}

void GameSuccess::GenerateFpsTextures()
{
	_fpsTextures.clear();

	for (int i = 0; i <= 1000; ++i)
	{
		std::string text = std::to_string(i);
		constexpr SDL_Color textColor = {140, 0, 255, 255};

		SDL_Surface* surface = TTF_RenderText_Solid(_fpsFont.get(), text.c_str(), textColor);
		if (!surface)
		{
			SDL_Log("Failed to create surface for FPS %d: %s", i, SDL_GetError());
			continue;
		}

		SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer.get(), surface);
		SDL_FreeSurface(surface);

		if (!texture)
		{
			SDL_Log("Failed to create texture for FPS %d: %s", i, SDL_GetError());
			continue;
		}

		_fpsTextures[i] = std::shared_ptr<SDL_Texture>(texture, SDL_DestroyTexture);
	}
}

void GameSuccess::CountFpsAndDeltaTime(float& deltaTime, Uint64& startFrameTime, const Uint64& endFrameTime)
{
	static Uint64 lastUpdate{0};
	static Uint32 lastDisplayedFps{0};
	static const Uint64 frequency{SDL_GetPerformanceFrequency()};

	const Uint64 frameDelta = endFrameTime - startFrameTime;
	deltaTime = frameDelta / static_cast<float>(frequency);

	//Cap to 60 FPS
	if (constexpr double targetFrameTime = 1.f / 60.f;
		!_isVsyncOn && deltaTime < targetFrameTime)
	{
		SDL_Delay(static_cast<Uint32>((targetFrameTime - deltaTime) * 1000));
		deltaTime = targetFrameTime;
	}

	if (const Uint64 timeSinceLastUpdate = endFrameTime - lastUpdate;
		timeSinceLastUpdate >= frequency)
	{
		const int fps = static_cast<int>(std::round(static_cast<double>(frequency) / static_cast<double>(frameDelta)));
		// SDL_Log("FPS %i", fps);

		if (fps != lastDisplayedFps
		    && _fpsTextures.contains(fps))
		{
			lastDisplayedFps = fps;
			_fpsTexture = _fpsTextures[fps];
		}

		lastUpdate = endFrameTime;
	}

	startFrameTime = endFrameTime;
}

void GameSuccess::DisposeDeadObject()
{
	const auto it = std::ranges::remove_if(_allObjects, [](const auto& obj) { return !obj->GetIsAlive(); }).begin();
	_allObjects.erase(it, _allObjects.end());
}

//TODO: recheck rule of 3/5 for all classes
//TODO: convert enum to enum classes

void GameSuccess::MainLoop()
{
	try
	{
		Uint64 startFrameTime = SDL_GetPerformanceCounter();
		float deltaTime{0.f};
		const SDL_Rect fpsRectangle{.x = static_cast<int>(_window->size.x) - 80, .y = 20, .w = 40, .h = 40};
		Uint64 endFrameTime{0u};
		while (!_userInput.IsGameOver())
		{
			CountFpsAndDeltaTime(deltaTime, startFrameTime, endFrameTime);

			_window->ClearBuffer();

			_userInput.Update();

			if (!_userInput.IsPause() && _gameMode != PlayAsClient)
			{
				//TODO: adjust timers on pause\unpause because it can be skipped like timer bonus
				_events->EmitEvent<const float>("TickUpdate", deltaTime);
			}

			DisposeDeadObject();

			_events->EmitEvent("RespawnTanks");

			_events->EmitEvent("Draw");

			_events->EmitEvent("DrawHealthBar");// TODO: blend separate buff layers(objects, effect, interface)

			// update screen with buffer
			SDL_UpdateTexture(_screen.get(), nullptr, _window->buffer.get(), static_cast<int>(_window->size.x) << 2);
			SDL_RenderCopy(_renderer.get(), _screen.get(), nullptr, nullptr);

			_events->EmitEvent("DrawTexture");

			_events->EmitEvent("DrawMenu");

			// Copy the texture with FPS to the renderer
			SDL_RenderCopy(_renderer.get(), _fpsTexture.get(), nullptr, &fpsRectangle);

			SDL_RenderPresent(_renderer.get());

			endFrameTime = SDL_GetPerformanceCounter();//TODO: change to system steady clock
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "error ..." << '\n';
	}
}

int GameSuccess::Result() const { return 0; }

GameMode GameSuccess::GetCurrentGameMode() const { return _gameMode; }

void GameSuccess::SetCurrentGameMode(const GameMode selectedGameMode)
{
	_gameMode = selectedGameMode;

	_events->EmitEvent<const GameMode>("GameModeChangedTo", _gameMode);
}
