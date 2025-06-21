#include "../../headers/application/GameSuccess.h"
#include "../../headers/BaseObj.h"
#include "../../headers/Map.h"
#include "../../headers/application/userInput.h"
#include "../../headers/components/BonusEffectManager.h"
#include "../../headers/components/BonusSpawner.h"
#include "../../headers/components/BulletPool.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/components/Menu.h"
#include "../../headers/components/ObstacleSpawner.h"
#include "../../headers/components/TankSpawner.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/network/ClientHandler.h"
#include "../../headers/network/ServerHandler.h"
#include <algorithm>
//#include <fstream>
#include <iostream>
#include <memory>
#include <boost/uuid/uuid_io.hpp>

//#ifdef _WIN32
//#define _WIN32_WINNT 0x0A00
//#endif
#define ASIO_STANDALONE

Uint32 FrameTimerCallback(Uint32 /*interval*/, void* param)
{
	const auto frameReady = static_cast<bool*>(param);
	*frameReady = true;

	return 0;
}

class BaseObj;
// std::ofstream error_log_server("error_log_Server.txt");
GameSuccess::GameSuccess(const UPoint windowSize, std::shared_ptr<SDL_Renderer> renderer,
                         std::shared_ptr<SDL_Texture> screen, std::shared_ptr<TTF_Font> fpsFont,
                         std::shared_ptr<EventSystem> events, std::shared_ptr<GameStatistics> statistics,
                         std::unique_ptr<Menu> menu, std::shared_ptr<TextureManager> textureManager,
                         const bool isVsyncOn, std::shared_ptr<BonusEffectManager> bonusEffectManager)
	: _selectedGameMode{OnePlayer},
	  _windowSize{windowSize},
	  _menu{std::move(menu)},
	  _statistics{std::move(statistics)},
	  _renderer{std::move(renderer)},
	  _screen{std::move(screen)},
	  _fpsFont{std::move(fpsFont)},
	  _events{events},
	  _bulletPool{std::make_shared<BulletPool>(events, &_allObjects, windowSize, Demo)},
	  _textureManager(std::move(textureManager)),
	  _userInput{std::make_shared<UserInput>(windowSize, events)},
	  _tankSpawner{
			  std::make_shared<TankSpawner>(windowSize, &_allObjects, events, _bulletPool,
			                                std::move(bonusEffectManager))},
	  _bonusSpawner{std::make_shared<BonusSpawner>(events, &_allObjects, windowSize)},
	  _obstacleSpawner{std::make_shared<ObstacleSpawner>(events, &_allObjects)},
	  _isVsyncOn{isVsyncOn},
	  _targetFrameDuration{1.0 / static_cast<double>(_targetFPS)}
{
	GenerateFpsTextures();

	Subscribe();

	ResetBattlefield(Demo);
}

GameSuccess::~GameSuccess()
{
	if (_frameTimer)
	{
		SDL_RemoveTimer(_frameTimer);
		_frameTimer = 0;
	}

	Unsubscribe();
}

void GameSuccess::Subscribe()
{
	_events->AddListener("PreviousGameMode", _name, [this]() { this->PrevGameMode(); });
	_events->AddListener("ClientReadyToStartGame", _name, [this]()
	{
		this->OnClientReady();
	});
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
		else
		{
			_networkNode = nullptr;
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

void GameSuccess::LoadMap() const
{
	//Map creation
	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	const Map field{_obstacleSpawner};//TODO: replace with obstacleSpawner->mapLoad(map)
	field.MapCreation(gridOffset);
}

void GameSuccess::ResetBattlefield(const GameMode gameMode)
{
	if (gameMode == PlayAsClient || gameMode == PlayAsHost)
	{
		_events->EmitEvent("Pause_Released");//NOTE: pause on start for awaiting a client ready
	}

	_allObjects.clear();
	_allObjects.reserve(1000);

	SetCurrentGameMode(gameMode);

	_events->EmitEvent("Reset");//TODO: recheck reset for new components

	if (gameMode != PlayAsClient && gameMode != PlayAsHost)
	{
		LoadMap();
	}

	if (gameMode == PlayAsClient)
	{
		_events->EmitEvent("ClientReadyToPlay");
	}
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

void GameSuccess::CountFpsAndDeltaTime(float& deltaTime,
                                       const std::chrono::high_resolution_clock::time_point& startFrameTime)
{
	static auto lastFpsUpdate = std::chrono::high_resolution_clock::now();
	static Uint32 lastDisplayedFps{0};
	static Uint32 frameCounter{0};

	std::chrono::high_resolution_clock::time_point endFrameTime = std::chrono::high_resolution_clock::now();
	auto frameDuration = std::chrono::duration<double>(endFrameTime - startFrameTime);
	deltaTime = static_cast<float>(frameDuration.count());

	if (!_isVsyncOn)
	{
		if (const auto timeToWait = _targetFrameDuration - frameDuration;
			timeToWait.count() > 0)
		{
			_frameReady = false;
			const Uint32 waitMs = static_cast<Uint32>(timeToWait.count() * 1000.0);
			_frameTimer = SDL_AddTimer(waitMs, FrameTimerCallback, &_frameReady);
			if (waitMs > 5)
			{
				SDL_Delay(waitMs - 5);
			}

			while (!_frameReady)
			{
				SDL_PumpEvents();
			}

			if (_frameTimer)
			{
				SDL_RemoveTimer(_frameTimer);
				_frameTimer = 0;
			}

			endFrameTime = std::chrono::high_resolution_clock::now();
			frameDuration = std::chrono::duration<double>(endFrameTime - startFrameTime);
			deltaTime = static_cast<float>(frameDuration.count());
		}
	}

	frameCounter++;
	if (const auto timeSinceLastFpsUpdate = std::chrono::duration<double>(endFrameTime - lastFpsUpdate);
		timeSinceLastFpsUpdate.count() >= 1.0)
	{
		const Uint32 fps = static_cast<Uint32>(std::round(frameCounter / timeSinceLastFpsUpdate.count()));
		frameCounter = 0;
		lastFpsUpdate = endFrameTime;

		if (fps != lastDisplayedFps)
		{
			if (const Uint32 cappedFps = std::min(fps, 1000u);
				_fpsTextures.contains(cappedFps))
			{
				lastDisplayedFps = fps;
				_fpsTexture = _fpsTextures[cappedFps];
			}
		}
	}
}

void GameSuccess::DisposeDeadObject()
{
	const auto it = std::ranges::remove_if(_allObjects, [](const auto& obj)
	{
		if (obj.get() == nullptr || obj.use_count() < 1)
		{
			return true;
		}

		return !obj->GetIsAlive();
	}).begin();

	//TODO: run on debug only
	for (auto itCopy = it; itCopy != _allObjects.end(); ++itCopy)
	{
		if (itCopy->get() == nullptr)
		{
			std::cout << "Disposing object nullptr " << std::endl;
			continue;
		}
		const auto baseObj = *itCopy;
		std::cout << "[" << "Disposing object" << "] "
				<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
				<< ", name=" << baseObj->GetName()
				<< ", UUID=" << boost::uuids::to_string(baseObj->GetUuid())
				<< std::endl;
	}

	_allObjects.erase(it, _allObjects.end());
}

//TODO: recheck rule of 3/5 for all classes
//TODO: convert enum to enum classes

void GameSuccess::OnClientReady() const
{
	LoadMap();
	this->_events->EmitEvent("Pause_Released");
}

void GameSuccess::MainLoop()
{
	try
	{
		float deltaTime{0.f};
		const SDL_Rect fpsRectangle{.x = static_cast<int>(_windowSize.x) - 80, .y = 20, .w = 40, .h = 40};
		while (!_userInput->IsGameOver())
		{
			std::chrono::high_resolution_clock::time_point startFrameTime = std::chrono::high_resolution_clock::now();

			if (_gameMode == PlayAsHost)
			{
				_events->EmitEvent("Server_StartFrame");
			}

			SDL_SetRenderDrawColor(_renderer.get(), 0, 0, 0, 255);
			SDL_RenderClear(_renderer.get());

			_userInput->Update();

			_menu->MenuUpdate();

			if (!_userInput->IsPause())
			{
				DisposeDeadObject();

				if (_gameMode != PlayAsClient)
				{
					//TODO: adjust timers on pause\unpause because it can be skipped like timer bonus
					_events->EmitEvent<const float>("TickUpdate", deltaTime);

					_tankSpawner->RespawnTanks();
				}
			}

			_events->EmitEvent("Draw");
			//TODO: optimize draw call with separated layer for brick, create image layer with all level brick, then when brick die replace it spot on layer with black rectangle

			_events->EmitEvent("DrawHealthBar");// TODO: blend separate buff layers(objects, effect, interface)

			_menu->DrawMenu();//TODO: optimize draw call with cache non changed text part

			// Copy the texture with FPS to the renderer
			SDL_RenderCopy(_renderer.get(), _fpsTexture.get(), nullptr, &fpsRectangle);

			SDL_RenderPresent(_renderer.get());

			if (_gameMode == PlayAsHost)
			{
				_events->EmitEvent("Server_EndFrame");
			}

			CountFpsAndDeltaTime(deltaTime, startFrameTime);
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
