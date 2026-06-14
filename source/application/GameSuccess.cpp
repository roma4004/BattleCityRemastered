#include "application/GameSuccess.h"
#include "application/UserInput.h"
#include "components/EventSystem.h"
#include "components/Menu.h"
#include "components/ScoreBoard.h"
#include "components/managers/BonusEffectManager.h"
#include "components/managers/FramePerSecondManager.h"
#include "components/managers/RenderManager.h"
#include "components/managers/SpawnManager.h"
#include "components/managers/StateManager.h"
#include "components/managers/TextureManager.h"
#include "enums/GameMode.h"
#include "network/ClientHandler.h"
#include "network/ServerHandler.h"
#include <algorithm>
#include <iostream>
#include <memory>
//#include <fstream>
#include <boost/uuid/uuid_io.hpp>

class BaseObj;

// std::ofstream error_log_server("error_log_Server.txt");
GameSuccess::GameSuccess(const UPoint windowSize, const std::shared_ptr<EventSystem>& events,
						 std::unique_ptr<Menu>& menu, const bool isVsyncOn,
						 std::unique_ptr<RenderManager>& renderManager,
						 std::unique_ptr<RightSideBar>& rightSideBar)
	: _windowSize{windowSize}
	, _menu{std::move(menu)}
	, _textureManager(std::make_unique<TextureManager>(windowSize, events))
	, _stateManager{std::make_unique<StateManager>(events)}
	, _userInput{std::make_unique<UserInput>(windowSize, events)}
	, _fpsManager{std::make_unique<FramePerSecondManager>(events, isVsyncOn)}
	, _spawnManager{std::make_unique<SpawnManager>(events, &_allObjects, windowSize)}
	, _renderManager{std::move(renderManager)}
	, _bonusEffectManager{std::make_unique<BonusEffectManager>(events)}
	, _scoreBoard{std::make_unique<ScoreBoard>(windowSize, events)}
	, _rightSideBar{std::move(rightSideBar)}
	, _events{events}
	, _selectedGameMode{GameMode::OnePlayer}
{
	Subscribe();

	ResetBattlefieldTo(GameMode::Demo);
	_events->EmitEvent("ShowMenu", true);
}

GameSuccess::~GameSuccess()
{
	Unsubscribe();
}

void GameSuccess::Subscribe()
{
	_events->AddListener("PreviousGameMode", _name, [this]() { this->PrevGameMode(); });
	_events->AddListener("NextGameMode", _name, [this]() { this->NextGameMode(); });
	_events->AddListener("ResetBattlefield", _name, [this]() { this->ResetBattlefieldTo(this->_selectedGameMode); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->OnGameModeChangedTo(newGameMode);
	});
	_events->AddListener("PostTickUpdate", _name, [this](const double /*deltaTime*/) { this->DisposeDeadObject(); });
	_events->AddListener("DeltaTime", _name, [this](const double& deltaTime) { this->_deltaTime = deltaTime; });
}

void GameSuccess::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void GameSuccess::ResetBattlefieldTo(const GameMode gameMode)
{
	_allObjects.clear();
	_allObjects.reserve(1000);

	_events->EmitEvent("Reset");

	SetCurrentGameMode(gameMode);

	if (gameMode != GameMode::PlayAsClient && gameMode != GameMode::PlayAsHost)
	{
		_events->EmitEvent("LoadMap");//TODO: move to obstacle spawner which should spawn when unpause 
	}

	if (gameMode == GameMode::PlayAsClient)
	{
		_events->EmitEvent("ClientSend_ReadyToPlay");
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
	_events->EmitEvent("LoadMap");
	_events->EmitEvent("Pause_Released");
}

void GameSuccess::MainLoop()
{
	try
	{
		while (!_userInput->IsShutdown())
		{
			_events->EmitEvent("FrameStart");
			_events->EmitEvent("NetCommandUpdate", _deltaTime);
			_events->EmitEvent("PreTickUpdate", _deltaTime);

			if (!_userInput->IsPause())
			{
				if (_gameMode != GameMode::PlayAsClient)
				{
					_events->EmitEvent("RespawnTanks", _deltaTime);

					//TODO: postpone all spawn to next frame, spawn queue will be exec each frame before tick update
					//TODO: adjust timers on pause\unpause because it can be skipped like timer bonus or:
					//TODO: avoid ticking timers on pause (pause for active timers, like reload, bonuses, bonus effects)
					_events->EmitEvent("TickUpdate", _deltaTime);
				}
			}

			_events->EmitEvent("PostTickUpdate", _deltaTime);

			//TODO: fix crash on client when we add brick on first start, in the middle of draw executing
			_events->EmitEvent("PreDraw");
			_events->EmitEvent("Draw");
			_events->EmitEvent("PostDraw");

			_events->EmitEvent("RenderRightSideBar");
			//TODO: optimize draw call with separated layer for brick, create image layer with all level brick, then when brick die replace it spot on layer with black rectangle

			_events->EmitEvent("PreDrawUserInterface");
			_events->EmitEvent("DrawUserInterface");
			_events->EmitEvent("PostDrawUserInterface");


			if (_gameMode == GameMode::PlayAsHost)
			{
				_events->EmitEvent("Server_EndFrame");
			}

			_events->EmitEvent("CalculateActualFps");
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
		_events->EmitEvent("Pause_Released");//NOTE: pause on start for awaiting a client ready
		_events->AddListener("ServerReceive_ClientReadyToStartGame", _name, [this]() { this->OnClientReady(); });
		_networkNode = std::make_unique<network::commands::ServerHandler>(_events);
	}
	else if (_gameMode == GameMode::PlayAsClient)
	{
		_events->RemoveListener("ServerReceive_ClientReadyToStartGame", _name);
		_networkNode = std::make_unique<network::commands::ClientHandler>(_events);
	}
	else
	{
		_events->RemoveListener("ServerReceive_ClientReadyToStartGame", _name);
		_networkNode = nullptr;
	}
}
