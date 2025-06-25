#include "application/GameSuccess.h"
#include "application/userInput.h"
#include "components/BonusEffectManager.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/Map.h"
#include "components/Menu.h"
#include "components/ObstacleSpawner.h"
#include "components/TankSpawner.h"
#include "entities/BaseObj.h"
#include "enums/GameMode.h"
#include "network/ClientHandler.h"
#include "network/ServerHandler.h"
#include <algorithm>
//#include <fstream>
#include "components/TextureManager.h"
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
GameSuccess::GameSuccess(const UPoint windowSize, std::shared_ptr<EventSystem> events, std::shared_ptr<GameStatistics> statistics,
                         std::unique_ptr<Menu> menu, std::shared_ptr<TextureManager> textureManager,
                         const bool isVsyncOn, std::shared_ptr<BonusEffectManager> bonusEffectManager)
	: _windowSize{windowSize},
	  _menu{std::move(menu)},
	  _statistics{std::move(statistics)},
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
	  _selectedGameMode{OnePlayer},
	  _targetFrameDuration{1.0 / static_cast<double>(_targetFps)}
{
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

Uint32 GameSuccess::CountFpsAndDeltaTime(float& deltaTime,
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
			lastDisplayedFps = std::min(fps, 1000u);
		}
	}

	return lastDisplayedFps;
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
		Uint32 fps{0};
		while (!_userInput->IsGameOver())
		{
			const auto startFrameTime = std::chrono::high_resolution_clock::now();

			if (_gameMode == PlayAsHost)
			{
				_events->EmitEvent("Server_StartFrame");
			}

			_textureManager->ClearFrame();

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

			_textureManager->DisplayFrame(fps);

			if (_gameMode == PlayAsHost)
			{
				_events->EmitEvent("Server_EndFrame");
			}

			fps = CountFpsAndDeltaTime(deltaTime, startFrameTime);
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
