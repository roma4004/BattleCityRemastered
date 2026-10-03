#pragma once

#include "components/EventSystem.h"
#include "components/MatchSettings.h"
#include <chrono>
#include <memory>
#include <vector>

enum class GameMode : char8_t;
struct LaunchOptions;
struct SDL_Config;
struct ApplyGameModeEvent;
struct ClientConnectedToHostEvent;
struct NextGameModeEvent;
struct PreviousGameModeEvent;
struct SelectedGameModeChangedToEvent;
struct ServerAddressChosenEvent;
class ServerProcess;
class EventSystem;
class FramePerSecondManager;
class GameConfig;
class LobbyScreen;
class Menu;
class ProjectConfig;
class RenderManager;
class RightSideBar;
class ScoreBoard;
class ServerScreen;
class Simulation;
class TextureManager;
class UiRenderer;
class UserInput;
class WindowConfig;

namespace network
{
class DiscoveryProbe;
}

//NOTE: the screen half - a Simulation plus everything that shows it; BattleCityServer skips it
class Game final
{
public:
	Game(GameConfig& gameConfig, const ProjectConfig& projectConfig, const WindowConfig& windowConfig,
		 SDL_Config& sdlConfig, const LaunchOptions& launchOptions);
	~Game();

	Game(const Game&) = delete;
	Game(Game&&) = delete;
	Game& operator=(const Game&) = delete;
	Game& operator=(Game&&) = delete;

	void Run();

	[[nodiscard]] static int Result();

private:
	void Subscribe();

	void PrevGameMode(const PreviousGameModeEvent&);
	void NextGameMode(const NextGameModeEvent&);
	void OnApplyGameMode(const ApplyGameModeEvent&);
	void OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event);
	void OnConnectedToHost(const ClientConnectedToHostEvent&);

	void OnServerAddressChosen(const ServerAddressChosenEvent& event);
	void EnterGameMode(GameMode mode);
	void AbandonHosting();

	//NOTE: a port nobody named, and our own child's, is asked of the server's beacon or read from the file it
	//writes next to the exe. It is watched rather than read once, so the two windows may be started in either order
	void WatchPublishedPort(GameMode mode);
	void PollPublishedPort();
	//NOTE: the answer matters only to the poll - the first reading is taken for its side effect
	bool TryAdoptPublishedPort();

	std::shared_ptr<EventSystem> _events{nullptr};

	std::unique_ptr<ServerProcess> _serverProcess{nullptr};
	std::unique_ptr<Menu> _menu{nullptr};
	std::unique_ptr<TextureManager> _textureManager{nullptr};
	std::unique_ptr<UserInput> _userInput{nullptr};
	std::unique_ptr<FramePerSecondManager> _fpsManager{nullptr};
	std::unique_ptr<Simulation> _simulation{nullptr};
	std::unique_ptr<RenderManager> _renderManager{nullptr};
	std::unique_ptr<UiRenderer> _uiRenderer{nullptr};
	std::unique_ptr<ScoreBoard> _scoreBoard{nullptr};
	std::unique_ptr<LobbyScreen> _lobbyScreen{nullptr};
	std::unique_ptr<RightSideBar> _rightSideBar{nullptr};
	std::unique_ptr<ServerScreen> _serverScreen{nullptr};

	std::vector<EventSubscription> _subs{};

	GameConfig& _gameConfig;

	GameMode _selectedGameMode{};

	//NOTE: what the server screen picked - a restart starts the server for it again
	MatchSettings _hostedMatch{};
	//NOTE: an address on the command line skips the server screen
	bool _isAddressNamedByArguments{};
	//NOTE: a port named on the command line or the server screen beats a published one - for a client only
	bool _isPortNamed{};
	bool _isDialingPublishedPort{};
	bool _isPortFileRead{};
	std::chrono::steady_clock::time_point _nextPortPoll{};
	//NOTE: built only while there is a port to look for, and dropped with the mode that wanted it
	std::unique_ptr<network::DiscoveryProbe> _portProbe{nullptr};
};
