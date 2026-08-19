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
struct GameModeSelectedWithMouseEvent;
struct PreviousGameModeEvent;
struct NextGameModeEvent;
struct ApplyGameModeEvent;
struct ServerInClientReadyToStartGameEvent;
struct ServerInDisconnectEvent;
struct ClientInDisconnectEvent;
struct GameModeChangedToEvent;
struct WorldGeometryChangedEvent;

class Game final
{
public:
	Game(GameConfig& gameConfig, const ProjectConfig& projectConfig, SDL_Config& sdlConfig, GameMode gameMode);

	//NOTE: defaulted out-of-line in the .cpp (not here) - this header only forward-declares the
	//manager types held by unique_ptr below, so an in-header default would need them complete here.
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
	void ApplyGameMode(GameMode gameMode);
	void PrevGameMode(const PreviousGameModeEvent&);
	void NextGameMode(const NextGameModeEvent&);
	void OnApplyGameMode(const ApplyGameModeEvent&);

	void OnAddToSpawnQueue(const AddToSpawnQueueEvent& event);
	void OnPostTickUpdate(const PostTickUpdateEvent&);
	void OnDeltaTime(const DeltaTimeEvent& event);
	void OnGameModeSelectedWithMouse(const GameModeSelectedWithMouseEvent& event);

	void DisposeDeadObject();
	void FlushSpawnQueue();

	void OnWorldGeometryChanged(const WorldGeometryChangedEvent& event);

	void OnClientReady(const ServerInClientReadyToStartGameEvent&);
	void OnClientLeft(const ServerInDisconnectEvent& event);
	void OnHostLeft(const ClientInDisconnectEvent& event);

	[[nodiscard]] GameMode GetCurrentGameMode() const;
	void SetCurrentGameMode(GameMode selectedGameMode);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);

	UPoint _windowSize{};

	//NOTE: declared before every manager - they take it in their constructors, and declaration order
	//is what decides both who is built first and who is torn down last
	std::shared_ptr<EventSystem> _events{nullptr};

	std::unique_ptr<INetworkNode> _networkNode{nullptr};
	std::unique_ptr<Menu> _menu{nullptr};
	std::unique_ptr<TextureManager> _textureManager{nullptr};
	std::unique_ptr<GameStateManager> _stateManager{nullptr};
	std::unique_ptr<UserInput> _userInput{nullptr};
	std::unique_ptr<FramePerSecondManager> _fpsManager{nullptr};
	//NOTE: before the spawners - the first LoadMap has to find a cell size already settled
	std::unique_ptr<WorldScaleManager> _worldScaleManager{nullptr};
	std::unique_ptr<SpawnManager> _spawnManager{nullptr};
	std::unique_ptr<RenderManager> _renderManager{nullptr};
	std::unique_ptr<BonusEffectManager> _bonusEffectManager{nullptr};
	std::unique_ptr<ScoreBoard> _scoreBoard{nullptr};
	std::unique_ptr<RightSideBar> _rightSideBar{nullptr};

	std::vector<EventSubscription> _subs{};
	// Toggled at runtime on every GameModeChangedToEvent (host-only listener), independent of
	// _subs's fixed subscribe-once-at-construction lifetime - assigning a new EventSubscription
	// here auto-unsubscribes whatever was previously held.
	EventSubscription _clientReadySub{};
	// Same runtime-toggled lifetime as _clientReadySub: only the side that can actually be left
	// listens - the host for a leaving client, the client for a leaving host.
	std::vector<EventSubscription> _peerLeftSubs{};
	//TODO: modify only under mutex lock (main and network thread can add)
	std::vector<std::shared_ptr<BaseObj>> _allObjects{};
	std::vector<std::shared_ptr<BaseObj>> _pendingSpawns{};

	GameConfig& _gameConfig;

	GameMode _selectedGameMode{};
	GameMode _gameMode{};
	double _deltaTime{};
	//NOTE: PauseReleasedEvent is a toggle, so a repeated client-ready would re-pause and reload the map
	bool _isClientReadyHandled{false};
	//NOTE: the host's goodbye is delivered while the network node is mid-call on the game thread, and
	//leaving the mode destroys that very node - so the switch waits for the end of the frame
	bool _isReturnToMenuPending{false};
	//NOTE: same deferral, host side - wiping _allObjects mid-frame pulls it from under the tick
	bool _isBattlefieldResetPending{false};
};
