#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <vector>

enum class GameMode : char8_t;
struct LaunchOptions;
struct SDL_Config;
struct ApplyGameModeEvent;
struct NextGameModeEvent;
struct PreviousGameModeEvent;
struct SelectedGameModeChangedToEvent;
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
class Simulation;
class TextureManager;
class UserInput;
class WindowConfig;

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

	[[nodiscard]] int Result() const;

private:
	void Subscribe();

	void PrevGameMode(const PreviousGameModeEvent&);
	void NextGameMode(const NextGameModeEvent&);
	void OnApplyGameMode(const ApplyGameModeEvent&);
	void OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event);

	void EnterGameMode(GameMode mode);

	std::shared_ptr<EventSystem> _events{nullptr};

	std::unique_ptr<ServerProcess> _serverProcess{nullptr};
	std::unique_ptr<Menu> _menu{nullptr};
	std::unique_ptr<TextureManager> _textureManager{nullptr};
	std::unique_ptr<UserInput> _userInput{nullptr};
	std::unique_ptr<FramePerSecondManager> _fpsManager{nullptr};
	std::unique_ptr<Simulation> _simulation{nullptr};
	std::unique_ptr<RenderManager> _renderManager{nullptr};
	std::unique_ptr<ScoreBoard> _scoreBoard{nullptr};
	std::unique_ptr<LobbyScreen> _lobbyScreen{nullptr};
	std::unique_ptr<RightSideBar> _rightSideBar{nullptr};

	std::vector<EventSubscription> _subs{};

	GameConfig& _gameConfig;

	GameMode _selectedGameMode{};
};
