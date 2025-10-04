#include "application/GameSuccess.h"
#include "application/userInput.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/Map.h"
#include "components/Menu.h"
#include "components/ObstacleSpawner.h"
#include "components/TankSpawner.h"
#include "components/managers/BonusEffectManager.h"
#include "enums/GameMode.h"
#include "entities/BaseObj.h"
#include "network/ClientHandler.h"
#include "network/ServerHandler.h"
#include <algorithm>
//#include <fstream>
#include "components/managers/TextureManager.h"
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

GameSuccess::GameSuccess(const UPoint windowSize, std::shared_ptr<EventSystem> events,
                         std::shared_ptr<GameStatistics> statistics, std::unique_ptr<Menu> menu,
                         std::shared_ptr<TextureManager> textureManager, const bool isVsyncOn,
                         std::shared_ptr<BonusEffectManager> bonusEffectManager,
                         std::shared_ptr<SpawnDelayManager> spawnDelayManager)
	: _windowSize{windowSize},
	  _menu{std::move(menu)},
	  _statistics{std::move(statistics)},
	  _events{events},
	  _bulletPool{std::make_shared<BulletPool>(events, &_allObjects, windowSize, GameMode::Demo)},
	  _textureManager(std::move(textureManager)),
	  _userInput{std::make_shared<UserInput>(windowSize, events)},
	  _tankSpawner{
			  std::make_shared<TankSpawner>(
					  windowSize, &_allObjects, events, _bulletPool, std::move(bonusEffectManager))},
	  _bonusSpawner{std::make_shared<BonusSpawner>(events, &_allObjects, windowSize)},
	  _obstacleSpawner{std::make_shared<ObstacleSpawner>(events, &_allObjects)},
	  _spawnDelayManager{std::move(spawnDelayManager)},
	  _isVsyncOn{isVsyncOn},
	  _selectedGameMode{GameMode::OnePlayer}
{
	_targetFrameDuration = std::chrono::duration<double>{1.0 / static_cast<double>(_targetFps)};
	Subscribe();

	ResetBattlefield(GameMode::Demo);
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
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->OnGameModeChangedTo(newGameMode);
	});
	_events->AddListener("DisposeStage", _name, [this]() { this->DisposeDeadObject(); });
}

void GameSuccess::Unsubscribe() const
{
	_events->RemoveListener("PreviousGameMode", _name);
	_events->RemoveListener("NextGameMode", _name);
	_events->RemoveListener("ResetBattlefield", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("DisposeStage", _name);
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
	if (gameMode == GameMode::PlayAsClient || gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("Pause_Released");//NOTE: pause on start for awaiting a client ready
	}

	_allObjects.clear();
	_allObjects.reserve(1000);

	SetCurrentGameMode(gameMode);

	_events->EmitEvent("Reset");//TODO: recheck reset for new components

	if (gameMode != GameMode::PlayAsClient && gameMode != GameMode::PlayAsHost)
	{
		LoadMap();
	}

	if (gameMode == GameMode::PlayAsClient)
	{
		_events->EmitEvent("ClientReadyToPlay");
	}
}

void GameSuccess::PrevGameMode()
{
	int mode = static_cast<int>(_selectedGameMode);
	--mode;

	constexpr int maxMode = static_cast<int>(GameMode::EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode < minMode ? maxMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent("SelectedGameModeChangedTo", _selectedGameMode);
}

void GameSuccess::NextGameMode()
{
	int mode = static_cast<int>(_selectedGameMode);
	++mode;

	constexpr int maxMode = static_cast<int>(GameMode::EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode > maxMode ? minMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent("SelectedGameModeChangedTo", _selectedGameMode);
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
			const auto waitMs = static_cast<Uint32>(timeToWait.count() * 1000.0);
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
		const auto fps = static_cast<Uint32>(std::round(frameCounter / timeSinceLastFpsUpdate.count()));
		frameCounter = 0;
		lastFpsUpdate = endFrameTime;

		if (fps != lastDisplayedFps)
		{
			lastDisplayedFps = std::min(fps, 1000u);
		}
	}

	return lastDisplayedFps;
}

// void GameSuccess::DisposeDeadObject()//TODO: run on debug only
// {
// 	auto predicate = [](const auto& obj) { return !obj.get() || !obj->GetIsAlive(); };
// 	const auto it = std::ranges::remove_if(_allObjects, predicate).begin();
//
// 	for (auto itCopy = it; itCopy != _allObjects.end(); ++itCopy)
// 	{
// 		if (*itCopy == nullptr)
// 		{
// 			std::cout << "Disposing object nullptr " << '\n';
// 			continue;
// 		}
// 		const auto& baseObj = *itCopy;
// 		std::cout << "[" << "Disposing object" << "] "
// 				<< "[" << (_gameMode == GameMode::PlayAsHost ? "SERVER" : "CLIENT") << "] "
// 				<< ", name=" << baseObj->GetName()
// 				<< ", UUID=" << boost::uuids::to_string(baseObj->GetUuid())
// 				<< '\n';
// 	}
//
// 	_allObjects.erase(it, _allObjects.end());
// }
void GameSuccess::DisposeDeadObject()
{
	std::erase_if(_allObjects, [](const auto& obj) { return obj.get() == nullptr || obj->GetIsAlive() == false; });
}

//TODO: recheck rule of 3/5 for all classes

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
		size_t fps{0};
		while (!_userInput->IsGameOver())
		{
			const auto startFrameTime = std::chrono::high_resolution_clock::now();

			if (_gameMode == GameMode::PlayAsHost)
			{
				_events->EmitEvent("Server_StartFrame");
			}

			_textureManager->ClearFrame();

			_userInput->Update();

			_menu->MenuUpdate();

			if (!_userInput->IsPause())
			{
				_events->EmitEvent("DisposeStage");

				if (_gameMode != GameMode::PlayAsClient)
				{
					//TODO: adjust timers on pause\unpause because it can be skipped like timer bonus
					_events->EmitEvent("TickUpdate", deltaTime);

					_tankSpawner->RespawnTanks();//TODO:split into two timers
				}
			}

			_events->EmitEvent("Draw");
			//TODO: optimize draw call with separated layer for brick, create image layer with all level brick, then when brick die replace it spot on layer with black rectangle

			_events->EmitEvent("AnimationUpdate");

			_events->EmitEvent("DrawHealthBar");// TODO: blend separate buff layers(objects, effect, interface)

			_menu->DrawMenu();//TODO: optimize draw call with cache non changed text part

			_textureManager->DisplayFrame(fps);

			if (_gameMode == GameMode::PlayAsHost)
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

	_events->EmitEvent("GameModeChangedTo", _gameMode);
}

void GameSuccess::OnGameModeChangedTo(const GameMode newGameMode)
{
	_gameMode = newGameMode;

	if (_gameMode == GameMode::PlayAsHost)
	{
		_networkNode = std::make_unique<ServerHandler>(_events);
	}
	else if (_gameMode == GameMode::PlayAsClient)
	{
		_networkNode = std::make_unique<ClientHandler>(_events);
	}
	else
	{
		_networkNode = nullptr;
	}
}
