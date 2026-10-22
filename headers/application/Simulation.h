#pragma once

#include "components/EventSystem.h"
#include "components/LevelRotation.h"
#include <cstdint>
#include <memory>
#include <vector>

enum class GameMode : char8_t;
struct ClientConnectedToHostEvent;
struct ClientInDisconnectEvent;
struct ClientReconnectAbandonedEvent;
struct ClientHostLostEvent;
struct DeltaTimeEvent;
struct GameModeChangedToEvent;
struct GameStateChangedToEvent;
struct MapLoadFailedEvent;
struct MatchStartedEvent;
struct NextLevelRequestedEvent;
struct PauseStatusEvent;
struct PlayerSlotAssignedEvent;
struct PostTickUpdateEvent;
class AnimationManager;
class BaseObj;
class BonusManager;
class EventSystem;
class GameConfig;
class GameStateManager;
class GameStatistics;
class INetworkNode;
class SpawnManager;
class WorldScaleManager;

//NOTE: the whole game minus the screen - BattleCityServer runs this alone, the game adds the screen
class Simulation final
{
	std::shared_ptr<EventSystem> _events{nullptr};

	std::unique_ptr<INetworkNode> _networkNode{nullptr};
	std::uint16_t _boundPort{};
	std::unique_ptr<GameStateManager> _stateManager{nullptr};
	//NOTE: a wall subscribes when it spawns, after every manager here, so it paints over the burst in any field order
	std::unique_ptr<AnimationManager> _animationManager{nullptr};
	std::unique_ptr<GameStatistics> _statistics{nullptr};
	std::unique_ptr<WorldScaleManager> _worldScaleManager{nullptr};
	std::unique_ptr<SpawnManager> _spawnManager{nullptr};
	std::unique_ptr<BonusManager> _bonusManager{nullptr};

	std::vector<EventSubscription> _subs{};

	GameConfig& _gameConfig;

	double _deltaTime{};
	double _stepAccumulator{};
	bool _isPaused{};
	//NOTE: the link and the lobby arrive in either order, so the ready waits for both
	bool _isLinkUp{};
	//NOTE: follows the phase, not latched - a match started later in the frame cancels it; handled at PostTickUpdate
	bool _isEnterLobbyPending{};
	//NOTE: set by StartNextLevel, read by the next GameResetEvent, which on a server waits for every ready
	bool _isNextLevel{};
	//NOTE: handled at PostTickUpdate - the reset must not clear the field inside NextLevelRequestedEvent's dispatch
	bool _isNextLevelPending{};

	LevelRotation _levels{};

	void Subscribe();

	void OnDeltaTime(const DeltaTimeEvent& event);
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnPostTickUpdate(const PostTickUpdateEvent&);
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnMatchStarted(const MatchStartedEvent&);
	void OnNextLevelRequested(const NextLevelRequestedEvent&);
	void OnMapLoadFailed(const MapLoadFailedEvent&) const;
	void OnConnectedToHost(const ClientConnectedToHostEvent&);
	void OnPlayerSlotAssigned(const PlayerSlotAssignedEvent& event);
	void OnHostLeft(const ClientInDisconnectEvent&);
	void OnHostUnreachable(const ClientReconnectAbandonedEvent&);
	void OnHostLost(const ClientHostLostEvent&);

	void EnterLobby();
	void StartNextLevel();
	void AnnounceReady() const;

public:
	Simulation(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~Simulation();

	Simulation(const Simulation&) = delete;
	Simulation(Simulation&&) = delete;
	Simulation& operator=(const Simulation&) = delete;
	Simulation& operator=(Simulation&&) = delete;

	[[nodiscard]] const GameStatistics& Statistics() const;
	[[nodiscard]] const std::vector<std::shared_ptr<BaseObj>>& World() const;
	[[nodiscard]] const AnimationManager& Animations() const;
	[[nodiscard]] const GameStateManager& State() const;

	//NOTE: drops the link only, so a caller can act between the old link going and the new one arriving
	void LeaveGameMode();

	void ApplyGameMode(GameMode gameMode);

	//NOTE: what the server ended up listening on - the same as asked for, unless the port was 0
	[[nodiscard]] std::uint16_t BoundPort() const noexcept { return _boundPort; }

	[[nodiscard]] bool TryRestartMatch() const;

	//NOTE: separate from SendNetworkFrame because drawing goes between them
	void Tick();

	void SendNetworkFrame() const;
};
