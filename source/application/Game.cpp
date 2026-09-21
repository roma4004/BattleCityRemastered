#include "application/Game.h"
#include "application/ProjectConfig.h"
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
#include "network/Endpoints.h"
#include <chrono>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>

namespace
{
//NOTE: a lobby screen away from being a user action, so it is polled rather than watched - slow enough
//to cost nothing, quick enough that the second window joins as soon as the first one is up
constexpr auto kPublishedPortPollStep{std::chrono::milliseconds{250}};
}//namespace

Game::Game(GameConfig& gameConfig, const ProjectConfig& projectConfig, const WindowConfig& windowConfig,
		   SDL_Config& sdlConfig, const LaunchOptions& launchOptions)
	: _events{std::make_shared<EventSystem>()}
	, _menu{std::make_unique<Menu>(_events, gameConfig)}
	, _textureManager(std::make_unique<TextureManager>(_events))
	, _userInput{std::make_unique<UserInput>(_events, windowConfig, sdlConfig, projectConfig.GamepadDeadZone())}
	, _fpsManager{std::make_unique<FramePerSecondManager>(_events, projectConfig, true)}
	, _simulation{std::make_unique<Simulation>(_events, gameConfig)}
	, _renderManager{std::make_unique<RenderManager>(_events, gameConfig, sdlConfig)}
	, _scoreBoard{std::make_unique<ScoreBoard>(_events, _simulation->Statistics())}
	, _lobbyScreen{std::make_unique<LobbyScreen>(_events, gameConfig)}
	, _rightSideBar{std::make_unique<RightSideBar>(_events, gameConfig)}
	, _gameConfig{gameConfig}
	, _selectedGameMode{GameMode::OnePlayer}
	, _isPortNamedByArguments{gameConfig.serverAddress.port != network::kAnyFreePort}
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
	_subs.push_back(_events->AddListener(this, &Game::OnConnectedToHost));
}

void Game::OnApplyGameMode(const ApplyGameModeEvent&) { EnterGameMode(_selectedGameMode); }

//NOTE: PlayAsHost starts BattleCityServer and joins it as an ordinary client - this process runs
//as a client either way, and takes whichever seat is free
void Game::EnterGameMode(const GameMode mode)
{
	//NOTE: both network entries apply PlayAsClient; only the server process tells them apart
	const GameMode entered{_serverProcess ? GameMode::PlayAsHost : _gameConfig.gameMode};
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
		WatchPublishedPort(mode);
		_simulation->ApplyGameMode(mode);

		return;
	}

	if (!_serverProcess)
	{
		_serverProcess = std::make_unique<ServerProcess>();
	}

	const std::optional<std::uint16_t> port{_serverProcess->Start(_gameConfig.serverAddress)};
	if (!port)
	{
		_serverProcess.reset();
		_events->EmitEvent(ShowMenuEvent{.show = true});

		return;
	}

	//NOTE: with --port=auto the child picked its own, so this is the first moment the game knows where to dial
	_gameConfig.serverAddress.port = *port;
	_isDialingPublishedPort = false;

	_simulation->ApplyGameMode(GameMode::PlayAsClient);
}

void Game::WatchPublishedPort(const GameMode mode)
{
	_isDialingPublishedPort = mode == GameMode::PlayAsClient && !_isPortNamedByArguments;
	if (!_isDialingPublishedPort)
	{
		return;
	}

	//NOTE: whatever number is left over belongs to a server we just shut down or to the last run - the
	//published file is the only one worth dialling, even when it says the same thing again
	_gameConfig.serverAddress.port = network::kAnyFreePort;
	_nextPortPoll = std::chrono::steady_clock::time_point{};

	TryAdoptPublishedPort();
}

void Game::PollPublishedPort()
{
	if (!_isDialingPublishedPort)
	{
		return;
	}

	const auto now{std::chrono::steady_clock::now()};
	if (now < _nextPortPoll)
	{
		return;
	}

	_nextPortPoll = now + kPublishedPortPollStep;

	if (!TryAdoptPublishedPort())
	{
		return;
	}

	Log::Info("joining the server published next to the game, port "
			  + std::to_string(_gameConfig.serverAddress.port));

	_simulation->ApplyGameMode(GameMode::PlayAsClient);
}

bool Game::TryAdoptPublishedPort()
{
	const std::optional<std::uint16_t> port{ServerProcess::PublishedPort()};
	if (!port || *port == _gameConfig.serverAddress.port)
	{
		return false;
	}

	_gameConfig.serverAddress.port = *port;

	return true;
}

//NOTE: the number held now is the one that answered, so there is nothing left to watch for
void Game::OnConnectedToHost(const ClientConnectedToHostEvent&) { _isDialingPublishedPort = false; }

void Game::OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event) { _selectedGameMode = event.mode; }

void Game::PrevGameMode(const PreviousGameModeEvent&)
{
	auto mode{static_cast<int>(_selectedGameMode)};
	--mode;

	constexpr int maxMode{static_cast<int>(GameMode::EndIterator) - 1};
	constexpr int minMode = 0;
	const int newMode{mode < minMode ? maxMode : mode};
	_selectedGameMode = static_cast<GameMode>(newMode);

	_events->EmitEvent(SelectedGameModeChangedToEvent{.mode = _selectedGameMode});
}

void Game::NextGameMode(const NextGameModeEvent&)
{
	auto mode{static_cast<int>(_selectedGameMode)};
	++mode;

	constexpr int maxMode{static_cast<int>(GameMode::EndIterator) - 1};
	constexpr int minMode = 0;
	const int newMode{mode > maxMode ? minMode : mode};
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
			PollPublishedPort();

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
