#include "application/GameSuccess.h"
#include "application/GameConfig.h"
#include "application/UserInput.h"
#include "components/EventSystem.h"
#include "components/Menu.h"
#include "components/RightSideBar.h"
#include "components/ScoreBoard.h"
#include "components/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/BonusEffectManager.h"
#include "components/managers/FramePerSecondManager.h"
#include "components/managers/GameStateManager.h"
#include "components/managers/RenderManager.h"
#include "components/managers/SpawnManager.h"
#include "components/managers/TextureManager.h"
#include "enums/GameMode.h"
#include "network/ClientHandler.h"
#include "network/ServerHandler.h"
#include <algorithm>
#include <iostream>
#include <memory>
//#include <fstream>

class BaseObj;

// std::ofstream error_log_server("error_log_Server.txt");
GameSuccess::GameSuccess(GameConfig& gameConfig, const std::shared_ptr<EventSystem>& events,
						 std::unique_ptr<Menu>& menu, std::unique_ptr<RenderManager>& renderManager,
						 const GameMode gameMode)
	: _menu{std::move(menu)}
	, _textureManager(std::make_unique<TextureManager>(events))
	, _stateManager{std::make_unique<GameStateManager>(events)}
	, _userInput{std::make_unique<UserInput>(gameConfig.windowSize, events, gameConfig)}
	, _fpsManager{std::make_unique<FramePerSecondManager>(events, gameConfig)}
	, _spawnManager{std::make_unique<SpawnManager>(events, &_allObjects, gameConfig)}
	, _renderManager{std::move(renderManager)}
	, _bonusEffectManager{std::make_unique<BonusEffectManager>(events)}
	, _scoreBoard{std::make_unique<ScoreBoard>(gameConfig.windowSize, events)}
	, _rightSideBar{std::make_unique<RightSideBar>(events)}
	, _events{events}
	, _selectedGameMode{GameMode::OnePlayer}
{
	Subscribe();

	ApplyGameMode(gameMode);

	if (gameMode == GameMode::Demo)
	{
		_events->EmitEvent(ShowMenuEvent{.show = true});
	}
}

GameSuccess::~GameSuccess() = default;

void GameSuccess::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &GameSuccess::PrevGameMode));
	_subs.push_back(_events->AddListener(this, &GameSuccess::NextGameMode));
	_subs.push_back(_events->AddListener(this, &GameSuccess::OnApplyGameMode));
	_subs.push_back(_events->AddListener(this, &GameSuccess::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &GameSuccess::OnAddToSpawnQueue));
	_subs.push_back(_events->AddListener(this, &GameSuccess::OnPostTickUpdate));
	_subs.push_back(_events->AddListener(this, &GameSuccess::OnDeltaTime));
	_subs.push_back(_events->AddListener(this, &GameSuccess::OnGameModeSelectedWithMouse));
}

void GameSuccess::OnApplyGameMode(const ApplyGameModeEvent&) { ApplyGameMode(_selectedGameMode); }

void GameSuccess::OnAddToSpawnQueue(const AddToSpawnQueueEvent& event) { _pendingSpawns.push_back(event.obj); }

void GameSuccess::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	FlushSpawnQueue();
	DisposeDeadObject();
}

void GameSuccess::OnDeltaTime(const DeltaTimeEvent& event) { _deltaTime = event.deltaTime; }

void GameSuccess::OnGameModeSelectedWithMouse(const GameModeSelectedWithMouseEvent& event)
{//TODO: merge with SelectedGameModeChangedToEvent
	_selectedGameMode = event.mode;
	_events->EmitEvent(SelectedGameModeChangedToEvent{.mode = _selectedGameMode});
}

void GameSuccess::ApplyGameMode(const GameMode gameMode)
{
	_allObjects.clear();
	_allObjects.reserve(1000);
	_pendingSpawns.clear();
	_isClientReadyHandled = false;

	_events->EmitEvent(GameResetEvent{});

	SetCurrentGameMode(gameMode);

	if (gameMode != GameMode::PlayAsClient && gameMode != GameMode::PlayAsHost)
	{
		_events->EmitEvent(LoadMapEvent{});//TODO: move to obstacle spawner which should spawn when unpause
	}

	if (gameMode == GameMode::PlayAsClient)
	{
		_events->EmitEvent(ClientOutReadyToPlayEvent{});
	}
}

void GameSuccess::PrevGameMode(const PreviousGameModeEvent&)
{
	int mode = static_cast<int>(_selectedGameMode);
	--mode;

	constexpr int maxMode = static_cast<int>(GameMode::EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode < minMode ? maxMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent(SelectedGameModeChangedToEvent{.mode = _selectedGameMode});
}

void GameSuccess::NextGameMode(const NextGameModeEvent&)
{
	int mode = static_cast<int>(_selectedGameMode);
	++mode;

	constexpr int maxMode = static_cast<int>(GameMode::EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode > maxMode ? minMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent(SelectedGameModeChangedToEvent{.mode = _selectedGameMode});
}

//TODO: push other tank mechanic like velosity with ice effect
// void GameSuccess::DisposeDeadObject()//TODO: add verbosity level for debug only ifndef
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

void GameSuccess::FlushSpawnQueue()
{
	_allObjects.insert(_allObjects.end(), std::make_move_iterator(_pendingSpawns.begin()),
					   std::make_move_iterator(_pendingSpawns.end()));
	_pendingSpawns.clear();
}

//TODO: recheck rule of 3/5 for all classes

void GameSuccess::OnClientReady(const ServerInClientReadyToStartGameEvent&)
{
	if (_isClientReadyHandled)
	{
		return;
	}

	_isClientReadyHandled = true;

	_events->EmitEvent(LoadMapEvent{});
	_events->EmitEvent(PauseReleasedEvent{});
}

void GameSuccess::MainLoop()
{
	try
	{
		while (!_userInput->IsShutdown())
		{
			_events->EmitEvent(FrameStartEvent{});
			_events->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTime});
			_events->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTime});

			if (!_userInput->IsPause())
			{
				if (_gameMode != GameMode::PlayAsClient)
				{
					constexpr bool skipDelay{false};
					_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});

					//TODO: adjust timers on pause\unpause because it can be skipped like timer bonus or:
					//TODO: avoid ticking timers on pause (pause for active timers, like reload, bonuses, bonus effects)
					_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTime});
				}
			}

			_events->EmitEvent(PostTickUpdateEvent{.deltaTime = _deltaTime});

			_events->EmitEvent(PreDrawEvent{});
			_events->EmitEvent(DrawEvent{});
			_events->EmitEvent(PostDrawEvent{});
			//TODO: optimize draw call with separated layer for brick, create image layer with all level brick, then when brick die replace it spot on layer with black rectangle

			_events->EmitEvent(PreDrawUserInterfaceEvent{});
			_events->EmitEvent(DrawUserInterfaceEvent{});
			_events->EmitEvent(PostDrawUserInterfaceEvent{});

			if (_gameMode == GameMode::PlayAsHost || _gameMode == GameMode::PlayAsClient)
			{
				_events->EmitEvent(NetworkEndFrameEvent{});
			}

			_events->EmitEvent(CalculateActualFpsEvent{});
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

	_events->EmitEvent(GameModeChangedToEvent{.mode = _gameMode});
}

void GameSuccess::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	_gameMode = event.mode;

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(PauseReleasedEvent{});//NOTE: pause on start for awaiting a client ready
		_clientReadySub = _events->AddListener(this, &GameSuccess::OnClientReady);
		_networkNode = std::make_unique<network::commands::ServerHandler>(_events);
	}
	else if (_gameMode == GameMode::PlayAsClient)
	{
		_clientReadySub = EventSubscription{};
		_networkNode = std::make_unique<network::commands::ClientHandler>(_events);
	}
	else
	{
		_clientReadySub = EventSubscription{};
		_networkNode = nullptr;
	}
}
