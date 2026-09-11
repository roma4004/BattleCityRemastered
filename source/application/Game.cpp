#include "application/Game.h"
#include "utils/Log.h"
#include "application/ServerProcess.h"
#include "application/GameConfig.h"
#include "application/LaunchOptions.h"
#include "application/Simulation.h"
#include "application/UserInput.h"
#include "application/WindowConfig.h"
#include "components/EventSystem.h"
#include "components/LobbyScreen.h"
#include "components/Menu.h"
#include "components/RightSideBar.h"
#include "components/ScoreBoard.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/FramePerSecondManager.h"
#include "components/managers/RenderManager.h"
#include "components/managers/TextureManager.h"
#include "enums/GameMode.h"
#include <exception>
#include <memory>

Game::Game(GameConfig& gameConfig, const ProjectConfig& projectConfig, const WindowConfig& windowConfig,
		   SDL_Config& sdlConfig, const LaunchOptions& launchOptions)
	: _events{std::make_shared<EventSystem>()}
	, _menu{std::make_unique<Menu>(_events, gameConfig)}
	, _textureManager(std::make_unique<TextureManager>(_events))
	, _userInput{std::make_unique<UserInput>(_events, windowConfig, sdlConfig)}
	, _fpsManager{std::make_unique<FramePerSecondManager>(_events, projectConfig, true)}
	, _simulation{std::make_unique<Simulation>(_events, gameConfig)}
	, _renderManager{std::make_unique<RenderManager>(_events, gameConfig, sdlConfig)}
	, _scoreBoard{std::make_unique<ScoreBoard>(_events, gameConfig, _simulation->Statistics())}
	, _lobbyScreen{std::make_unique<LobbyScreen>(_events, gameConfig)}
	, _rightSideBar{std::make_unique<RightSideBar>(_events, gameConfig)}
	, _gameConfig{gameConfig}
	, _selectedGameMode{GameMode::OnePlayer}
{
	Subscribe();

	EnterGameMode(launchOptions.gameMode);

	if (launchOptions.isDemo)
	{
		_events->EmitEvent(DemoStartedEvent{});
		_events->EmitEvent(ShowMenuEvent{.show = true});
	}
}

Game::~Game() = default;

void Game::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Game::PrevGameMode));
	_subs.push_back(_events->AddListener(this, &Game::NextGameMode));
	_subs.push_back(_events->AddListener(this, &Game::OnApplyGameMode));
	_subs.push_back(_events->AddListener(this, &Game::OnSelectedGameModeChangedTo));
}

void Game::OnApplyGameMode(const ApplyGameModeEvent&) { EnterGameMode(_selectedGameMode); }

//NOTE: PlayAsHost starts BattleCityServer and joins it as an ordinary client - this process runs
//as a client either way, and takes whichever seat is free
void Game::EnterGameMode(const GameMode mode)
{
	//NOTE: the applied mode says PlayAsClient for both network entries, and the server process is
	//what tells them apart
	const GameMode entered = _serverProcess ? GameMode::PlayAsHost : _gameConfig.gameMode;
	if (mode == entered && _simulation->TryRestartMatch())
	{
		return;
	}

	//NOTE: the link goes before the process it talks to - built any earlier it dials a server
	//this call is about to kill
	_simulation->LeaveGameMode();

	if (mode != GameMode::PlayAsHost)
	{
		_serverProcess.reset();
		_simulation->ApplyGameMode(mode);

		return;
	}

	if (!_serverProcess)
	{
		_serverProcess = std::make_unique<ServerProcess>();
	}

	if (!_serverProcess->Start())
	{
		_serverProcess.reset();
		_events->EmitEvent(ShowMenuEvent{.show = true});

		return;
	}

	_simulation->ApplyGameMode(GameMode::PlayAsClient);
}

void Game::OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event) { _selectedGameMode = event.mode; }

void Game::PrevGameMode(const PreviousGameModeEvent&)
{
	int mode = static_cast<int>(_selectedGameMode);
	--mode;

	constexpr int maxMode = static_cast<int>(GameMode::EndIterator) - 1;
	constexpr int minMode = 0;
	const int newMode = mode < minMode ? maxMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent(SelectedGameModeChangedToEvent{.mode = _selectedGameMode});
}

void Game::NextGameMode(const NextGameModeEvent&)
{
	int mode = static_cast<int>(_selectedGameMode);
	++mode;

	constexpr int maxMode = static_cast<int>(GameMode::EndIterator) - 1;
	constexpr int minMode = 0;
	const int newMode = mode > maxMode ? minMode : mode;
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent(SelectedGameModeChangedToEvent{.mode = _selectedGameMode});
}

//TODO: push other tank mechanic like velosity with ice effect

//TODO: recheck rule of 3/5 for all classes

void Game::Run()
{
	try
	{
		while (!_userInput->IsShutdown())
		{
			_simulation->Tick();

			_events->EmitEvent(PreDrawEvent{});
			_events->EmitEvent(DrawEvent{});
			_events->EmitEvent(PostDrawEvent{});
			//TODO: optimize draw call with separated layer for brick, create image layer with all level brick, then when brick die replace it spot on layer with black rectangle

			_events->EmitEvent(PreDrawUserInterfaceEvent{});
			_events->EmitEvent(DrawUserInterfaceEvent{});
			_events->EmitEvent(PostDrawUserInterfaceEvent{});

			_simulation->EndNetworkFrame();

			_events->EmitEvent(PresentFrameEvent{});

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
