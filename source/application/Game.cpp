#include "application/Game.h"
#include "utils/Log.h"
#include "application/GameConfig.h"
#include "application/UserInput.h"
#include "components/EventSystem.h"
#include "components/Menu.h"
#include "components/RightSideBar.h"
#include "components/ScoreBoard.h"
#include "components/events/SpawnEvents.h"
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
#include "components/managers/WorldScaleManager.h"
#include "enums/GameMode.h"
#include "network/ClientHandler.h"
#include "network/ServerHandler.h"
#include <cmath>
#include <memory>
//#include <fstream>

class BaseObj;

// std::ofstream error_log_server("error_log_Server.txt");
Game::Game(GameConfig& gameConfig, const ProjectConfig& projectConfig, SDL_Config& sdlConfig, const GameMode gameMode)
	: _events{std::make_shared<EventSystem>()}
	, _menu{std::make_unique<Menu>(_events, gameConfig)}
	, _textureManager(std::make_unique<TextureManager>(_events))
	, _stateManager{std::make_unique<GameStateManager>(_events)}
	, _userInput{std::make_unique<UserInput>(_events, gameConfig)}
	, _fpsManager{std::make_unique<FramePerSecondManager>(_events, projectConfig)}
	, _worldScaleManager{std::make_unique<WorldScaleManager>(_events, gameConfig)}
	, _spawnManager{std::make_unique<SpawnManager>(_events, &_allObjects, gameConfig)}
	, _renderManager{std::make_unique<RenderManager>(_events, gameConfig, sdlConfig)}
	, _bonusEffectManager{std::make_unique<BonusEffectManager>(_events)}
	, _scoreBoard{std::make_unique<ScoreBoard>(_events, gameConfig)}
	, _rightSideBar{std::make_unique<RightSideBar>(_events, gameConfig)}
	, _gameConfig{gameConfig}
	, _selectedGameMode{GameMode::OnePlayer}
{
	Subscribe();

	ApplyGameMode(gameMode);

	if (gameMode == GameMode::Demo)
	{
		_events->EmitEvent(ShowMenuEvent{.show = true});
	}
}

Game::~Game() = default;

void Game::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Game::PrevGameMode));
	_subs.push_back(_events->AddListener(this, &Game::NextGameMode));
	_subs.push_back(_events->AddListener(this, &Game::OnApplyGameMode));
	_subs.push_back(_events->AddListener(this, &Game::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &Game::OnAddToSpawnQueue));
	_subs.push_back(_events->AddListener(this, &Game::OnPostTickUpdate));
	_subs.push_back(_events->AddListener(this, &Game::OnDeltaTime));
	_subs.push_back(_events->AddListener(this, &Game::OnGameModeSelectedWithMouse));
	_subs.push_back(_events->AddListener(this, &Game::OnWorldGeometryChanged));
}

//NOTE: whoever already stands on the field keeps its place in cells, not in pixels - so every rect
//is scaled by how much the cell itself changed. Obstacles spawned after this already use the new one.
void Game::OnWorldGeometryChanged(const WorldGeometryChangedEvent& event)
{
	constexpr float kNoticeableCellChange{0.001f};
	if (event.previousCellSize <= 0.f
		|| std::abs(event.cellSize - event.previousCellSize) < kNoticeableCellChange)
	{
		return;
	}

	const float ratio = event.cellSize / event.previousCellSize;
	for (const std::shared_ptr<BaseObj>& obj: _allObjects)
	{
		if (!obj)
		{
			continue;
		}

		obj->SetPos(FPoint{.x = obj->GetX() * ratio, .y = obj->GetY() * ratio});
		obj->SetWidth(obj->GetWidth() * ratio);
		obj->SetHeight(obj->GetHeight() * ratio);
	}
}

void Game::OnApplyGameMode(const ApplyGameModeEvent&) { ApplyGameMode(_selectedGameMode); }

void Game::OnAddToSpawnQueue(const AddToSpawnQueueEvent& event) { _pendingSpawns.push_back(event.obj); }

void Game::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	FlushSpawnQueue();
	DisposeDeadObject();

	if (_isBattlefieldResetPending)
	{
		_isBattlefieldResetPending = false;
		ResetBattlefield();
		//NOTE: after the reset, never before - GameResetEvent clears the pause flag itself
		_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	}

	if (_isReturnToMenuPending)
	{
		_isReturnToMenuPending = false;
		ApplyGameMode(GameMode::Demo);
		_events->EmitEvent(ShowMenuEvent{.show = true});
	}
}

void Game::OnDeltaTime(const DeltaTimeEvent& event) { _deltaTime = event.deltaTime; }

void Game::OnGameModeSelectedWithMouse(const GameModeSelectedWithMouseEvent& event)
{//TODO: merge with SelectedGameModeChangedToEvent
	_selectedGameMode = event.mode;
	_events->EmitEvent(SelectedGameModeChangedToEvent{.mode = _selectedGameMode});
}

void Game::ResetBattlefield()
{
	_allObjects.clear();
	_allObjects.reserve(1000);
	_pendingSpawns.clear();

	_events->EmitEvent(GameResetEvent{});
}

void Game::ApplyGameMode(const GameMode gameMode)
{
	//NOTE: before the reset - listeners of GameResetEvent read the mode off the config, so it has
	//to be the new one already
	_gameConfig.gameMode = gameMode;

	ResetBattlefield();
	_isClientReadyHandled = false;
	_isBattlefieldResetPending = false;

	SetCurrentGameMode(gameMode);

	if (IsLocalGame(gameMode))
	{
		_events->EmitEvent(LoadMapEvent{});//TODO: move to obstacle spawner which should spawn when unpause
	}

	if (IsClient(gameMode))
	{
		_events->EmitEvent(ClientOutReadyToPlayEvent{});
	}
}

void Game::PrevGameMode(const PreviousGameModeEvent&)
{
	int mode = static_cast<int>(_selectedGameMode);
	--mode;

	constexpr int maxMode = static_cast<int>(GameMode::EndIterator) - 1;
	constexpr int minMode = 1;
	const int newMode = mode < minMode ? maxMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent(SelectedGameModeChangedToEvent{.mode = _selectedGameMode});
}

void Game::NextGameMode(const NextGameModeEvent&)
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

void Game::DisposeDeadObject()
{
	std::erase_if(_allObjects, [](const auto& obj) { return obj.get() == nullptr || obj->GetIsAlive() == false; });
}

void Game::FlushSpawnQueue()
{
	_allObjects.insert(_allObjects.end(), std::make_move_iterator(_pendingSpawns.begin()),
					   std::make_move_iterator(_pendingSpawns.end()));
	_pendingSpawns.clear();
}

//TODO: recheck rule of 3/5 for all classes

void Game::OnClientReady(const ServerInClientReadyToStartGameEvent&)
{
	if (_isClientReadyHandled)
	{
		return;
	}

	_isClientReadyHandled = true;

	_events->EmitEvent(LoadMapEvent{});
	_events->EmitEvent(PauseReleasedEvent{});
}

void Game::OnClientLeft(const ServerInDisconnectEvent&)
{
	//NOTE: back to the pre-game wait - the next client's ready reloads the map, which would stack
	//onto the running one if the field were kept
	_isClientReadyHandled = false;
	_isBattlefieldResetPending = true;
}

void Game::OnHostLeft(const ClientInDisconnectEvent&)
{
	_isReturnToMenuPending = true;
}

void Game::Run()
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
				if (IsAuthority(_gameMode))
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

			if (IsNetworkGame(_gameMode))
			{
				_events->EmitEvent(NetworkEndFrameEvent{});
			}

			_events->EmitEvent(CalculateActualFpsEvent{});
		}
	}
	catch (std::exception& e)
	{
		Log::Error(e.what());
	}
	catch (...)
	{
		Log::Error("unknown exception in the main loop");
	}
}

int Game::Result() const { return 0; }

GameMode Game::GetCurrentGameMode() const { return _gameMode; }

void Game::SetCurrentGameMode(const GameMode selectedGameMode)
{
	_gameMode = selectedGameMode;

	_events->EmitEvent(GameModeChangedToEvent{.mode = _gameMode});
}

void Game::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	_gameMode = event.mode;

	_peerLeftSubs.clear();

	//NOTE: the old node goes first - assigning over it would build the new one (same port, new
	//connect) while the outgoing one still holds both
	_networkNode.reset();

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(PauseReleasedEvent{});//NOTE: pause on start for awaiting a client ready
		_clientReadySub = _events->AddListener(this, &Game::OnClientReady);
		_peerLeftSubs.push_back(_events->AddListener(this, &Game::OnClientLeft));
		_networkNode = std::make_unique<network::commands::ServerHandler>(_events);
	}
	else if (IsClient(_gameMode))
	{
		_clientReadySub = EventSubscription{};
		_peerLeftSubs.push_back(_events->AddListener(this, &Game::OnHostLeft));
		_networkNode = std::make_unique<network::commands::ClientHandler>(_events);
	}
	else
	{
		_clientReadySub = EventSubscription{};
	}
}
