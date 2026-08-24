#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"
#include <chrono>

enum class GameMode : char8_t;
class INetworkNode;
class IDrawable;
class BaseObj;
class Menu;
class UserInput;
class TextureManager;
class GameStateManager;
class FramePerSecondManager;
class SpawnManager;
class WorldScaleManager;
class RenderManager;
class EventSystem;
class BonusEffectManager;
class ScoreBoard;
class GameStatistics;
class RightSideBar;
class GameConfig;
class ProjectConfig;
struct SDL_Config;
struct AddToSpawnQueueEvent;
struct PostTickUpdateEvent;
struct DeltaTimeEvent;
struct PreviousGameModeEvent;
struct NextGameModeEvent;
struct ApplyGameModeEvent;
struct ServerInClientReadyToStartGameEvent;
struct ServerInDisconnectEvent;
struct ClientInDisconnectEvent;
struct ClientReconnectAbandonedEvent;
struct GameStateChangedToEvent;
struct GameModeChangedToEvent;
struct SelectedGameModeChangedToEvent;
struct WorldGeometryChangedEvent;

class Game final
{
public:
	Game(GameConfig& gameConfig, const ProjectConfig& projectConfig, SDL_Config& sdlConfig, GameMode gameMode);
	~Game();

	Game(const Game&) = delete;
	Game(Game&&) = delete;
	Game& operator=(const Game&) = delete;
	Game& operator=(Game&&) = delete;

	void Run();

	[[nodiscard]] int Result() const;

private:
	void Subscribe();

	void ResetBattlefield();
	void EnterLobby();
	void ApplyGameMode(GameMode gameMode);
	void PrevGameMode(const PreviousGameModeEvent&);
	void NextGameMode(const NextGameModeEvent&);
	void OnApplyGameMode(const ApplyGameModeEvent&);

	void OnAddToSpawnQueue(const AddToSpawnQueueEvent& event);
	void OnPostTickUpdate(const PostTickUpdateEvent&);
	void OnDeltaTime(const DeltaTimeEvent& event);
	void OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event);

	void DisposeDeadObject();
	void FlushSpawnQueue();

	void OnWorldGeometryChanged(const WorldGeometryChangedEvent& event);

	void OnGameStateChangedTo(const GameStateChangedToEvent& event);

	[[nodiscard]] GameMode GetCurrentGameMode() const;
	void SetCurrentGameMode(GameMode selectedGameMode);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);

	UPoint _windowSize{};

	std::shared_ptr<EventSystem> _events{nullptr};

	std::unique_ptr<INetworkNode> _networkNode{nullptr};
	std::unique_ptr<Menu> _menu{nullptr};
	std::unique_ptr<TextureManager> _textureManager{nullptr};
	std::unique_ptr<GameStateManager> _stateManager{nullptr};
	std::unique_ptr<UserInput> _userInput{nullptr};
	std::unique_ptr<FramePerSecondManager> _fpsManager{nullptr};
	std::unique_ptr<WorldScaleManager> _worldScaleManager{nullptr};
	std::unique_ptr<SpawnManager> _spawnManager{nullptr};
	std::unique_ptr<RenderManager> _renderManager{nullptr};
	std::unique_ptr<BonusEffectManager> _bonusEffectManager{nullptr};
	std::unique_ptr<ScoreBoard> _scoreBoard{nullptr};
	std::unique_ptr<RightSideBar> _rightSideBar{nullptr};

	std::vector<EventSubscription> _subs{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects{};
	std::vector<std::shared_ptr<BaseObj>> _pendingSpawns{};

	GameConfig& _gameConfig;

	GameMode _selectedGameMode{};
	GameMode _gameMode{};
	double _deltaTime{};
	//NOTE: deferred to PostTickUpdate - the peer goes away mid-frame, and the field may be mid-iteration
	bool _isEnterLobbyPending{false};
};
