#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <vector>

enum class GameMode : char8_t;
struct ClientConnectedToHostEvent;
struct ClientInDisconnectEvent;
struct ClientReconnectAbandonedEvent;
struct DeltaTimeEvent;
struct GameModeChangedToEvent;
struct GameStateChangedToEvent;
struct MatchStartedEvent;
struct PauseStatusEvent;
struct PlayerSlotAssignedEvent;
struct PostTickUpdateEvent;
class AnimationManager;
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
	std::unique_ptr<GameStateManager> _stateManager{nullptr};
	//NOTE: before the spawner, so the burst paints under the walls - within one event order is subscription order
	std::unique_ptr<AnimationManager> _animationManager{nullptr};
	std::unique_ptr<GameStatistics> _statistics{nullptr};
	std::unique_ptr<WorldScaleManager> _worldScaleManager{nullptr};
	std::unique_ptr<SpawnManager> _spawnManager{nullptr};
	std::unique_ptr<BonusManager> _bonusManager{nullptr};

	std::vector<EventSubscription> _subs{};

	GameConfig& _gameConfig;

	double _deltaTime{};
	double _stepAccumulator{0.0};
	bool _isPaused{false};
	//NOTE: the link and the lobby arrive in either order, so the ready waits for both
	bool _isLinkUp{false};
	//NOTE: derived from the phase, not latched, so a match starting later in the same frame clears a
	//lobby entry that no longer applies. Acted on at PostTickUpdate
	bool _isEnterLobbyPending{false};

	void Subscribe();

	void OnDeltaTime(const DeltaTimeEvent& event);
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnPostTickUpdate(const PostTickUpdateEvent&);
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnMatchStarted(const MatchStartedEvent&);
	void OnConnectedToHost(const ClientConnectedToHostEvent&);
	void OnPlayerSlotAssigned(const PlayerSlotAssignedEvent& event);
	void OnHostLeft(const ClientInDisconnectEvent&);
	void OnHostUnreachable(const ClientReconnectAbandonedEvent&);

	void EnterLobby();
	void AnnounceReady() const;

public:
	Simulation(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~Simulation();

	Simulation(const Simulation&) = delete;
	Simulation(Simulation&&) = delete;
	Simulation& operator=(const Simulation&) = delete;
	Simulation& operator=(Simulation&&) = delete;

	[[nodiscard]] const GameStatistics& Statistics() const;

	//NOTE: drops the link only, so a caller can act between the old link going and the new one arriving
	void LeaveGameMode();

	void ApplyGameMode(GameMode gameMode);

	[[nodiscard]] bool TryRestartMatch() const;

	//NOTE: separate from EndNetworkFrame because drawing goes between them
	void Tick();

	void EndNetworkFrame() const;
};
