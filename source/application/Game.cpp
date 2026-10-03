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
#include "components/ServerScreen.h"
#include "components/UiRenderer.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/FramePerSecondManager.h"
#include "components/managers/RenderManager.h"
#include "components/managers/TextureManager.h"
#include "enums/GameMode.h"
#include "network/DiscoveryProbe.h"
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
	, _uiRenderer{std::make_unique<UiRenderer>(_events, gameConfig, sdlConfig)}
	, _scoreBoard{std::make_unique<ScoreBoard>(_events, _simulation->Statistics(), gameConfig)}
	, _lobbyScreen{std::make_unique<LobbyScreen>(_events, gameConfig)}
	, _rightSideBar{std::make_unique<RightSideBar>(_events, gameConfig)}
	, _gameConfig{gameConfig}
	, _selectedGameMode{GameMode::OnePlayer}
	, _isAddressNamedByArguments{launchOptions.serverHost.has_value()}
	, _isPortNamed{gameConfig.serverAddress.port != network::kAnyFreePort}
{
	//NOTE: loopback is out of reach from another machine
	if (!_isAddressNamedByArguments)
	{
		_gameConfig.serverAddress.host = network::LocalAddress();
	}

	_serverScreen = std::make_unique<ServerScreen>(_events, _gameConfig.serverAddress);

	Subscribe();

	EnterGameMode(launchOptions.gameMode);

	if (launchOptions.isDemo)
	{
		_events->EmitEvent(DemoStartedEvent{});
		_events->EmitEvent(ShowMenuEvent{.isShown = true});
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
	_subs.push_back(_events->AddListener(this, &Game::OnServerAddressChosen));
}

void Game::OnApplyGameMode(const ApplyGameModeEvent&)
{
	//NOTE: only the server process tells the two network entries apart; a restart needs no address
	const GameMode entered{_serverProcess ? GameMode::PlayAsHost : _gameConfig.gameMode};
	//NOTE: a client asked again picks its server anew, unless the command line named it
	const bool isPickedAnew{_selectedGameMode == GameMode::PlayAsClient && !_isAddressNamedByArguments};
	if (_selectedGameMode == entered && !isPickedAnew && _simulation->TryRestartMatch())
	{
		return;
	}

	if (IsNetworkGame(_selectedGameMode) && !_isAddressNamedByArguments)
	{
		_serverScreen->Open(_selectedGameMode);

		return;
	}

	EnterGameMode(_selectedGameMode);
}

//NOTE: a port the screen leaves out is looked up anew
void Game::OnServerAddressChosen(const ServerAddressChosenEvent& event)
{
	_gameConfig.serverAddress = event.address;
	_isPortNamed = event.address.port != network::kAnyFreePort;
	//NOTE: a server still up was started on the address before; the link goes first, as in EnterGameMode
	_simulation->LeaveGameMode();
	_serverProcess.reset();

	EnterGameMode(event.mode);
}

//NOTE: PlayAsHost starts BattleCityServer and joins it as an ordinary client - this process runs
//as a client either way, and takes whichever seat is free
void Game::EnterGameMode(const GameMode mode)
{
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

	if (!_serverProcess->Start(_gameConfig.serverAddress))
	{
		AbandonHosting();

		return;
	}

	//NOTE: the lobby comes up at once rather than the window freezing while the child starts
	WatchPublishedPort(mode);
	_simulation->ApplyGameMode(GameMode::PlayAsClient);
}

void Game::WatchPublishedPort(const GameMode mode)
{
	//NOTE: our own server is dialled by the port it writes down even when it was named - it listens only then
	_isDialingPublishedPort = mode == GameMode::PlayAsHost || (mode == GameMode::PlayAsClient && !_isPortNamed);
	//NOTE: the file lies next to this exe, so it can only name a server on this machine
	_isPortFileRead = mode == GameMode::PlayAsHost || network::IsThisMachine(_gameConfig.serverAddress.host);
	if (!_isDialingPublishedPort)
	{
		//NOTE: dropped with the mode that wanted it - its socket and thread have nobody to ask any more
		_portProbe.reset();

		return;
	}

	//NOTE: whatever number is left over belongs to a server we just shut down or to the last run - only
	//what a server says now is worth dialling, even when it says the same thing again
	_gameConfig.serverAddress.port = network::kAnyFreePort;
	_nextPortPoll = std::chrono::steady_clock::time_point{};
	//NOTE: a beacon on this machine may be another server's - our own child is read from its file
	_portProbe = mode == GameMode::PlayAsHost
						 ? nullptr
						 : std::make_unique<network::DiscoveryProbe>(_gameConfig.serverAddress.host);

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

	//NOTE: a child that died says so at once - the lobby would wait for its port for ever
	if (_serverProcess && !_serverProcess->IsRunning())
	{
		Log::Error("ServerProcess: BattleCityServer exited before the game joined it");
		AbandonHosting();

		return;
	}

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
	//NOTE: the beacon first - it answers over the network, so it works for a server on another machine,
	//while the file rides on both copies sitting in the same folder
	std::optional<std::uint16_t> port{};
	if (_portProbe)
	{
		if (const auto reply{_portProbe->Poll()})
		{
			port = reply->gamePort;
		}
	}

	if (!port && _isPortFileRead)
	{
		port = ServerProcess::PublishedPort();
	}

	if (!port || *port == _gameConfig.serverAddress.port)
	{
		return false;
	}

	_gameConfig.serverAddress.port = *port;

	return true;
}

//NOTE: back to the menu, with nothing left dialling a server that is not there
void Game::AbandonHosting()
{
	_serverProcess.reset();
	_isDialingPublishedPort = false;
	_portProbe.reset();
	_simulation->LeaveGameMode();
	_events->EmitEvent(ShowMenuEvent{.isShown = true});
}

//NOTE: the number held now is the one that answered, so there is nothing left to watch for
void Game::OnConnectedToHost(const ClientConnectedToHostEvent&)
{
	_isDialingPublishedPort = false;
	_portProbe.reset();
}

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

int Game::Result() { return 0; }
