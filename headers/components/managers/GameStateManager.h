#pragma once

#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include <bitset>
#include <cstdint>
#include <memory>
#include <vector>

enum class PlayerSlot : std::uint8_t;
struct PauseStatusEvent;
struct PreDrawUserInterfaceEvent;
struct PostDrawUserInterfaceEvent;
struct ScoreBoardShownEvent;
struct GameResetEvent;
struct GameModeAppliedEvent;
struct DemoStartedEvent;
struct GameFinishedEvent;
struct MapLoadFailedEvent;
struct ServerInClientReadyToStartGameEvent;
struct ServerInRestartRequestedEvent;
struct MatchRestartRequestedEvent;
struct HostPhaseAnnouncedEvent;
struct ServerInDisconnectEvent;
struct ServerClientLostEvent;
struct ClientInDisconnectEvent;
struct ClientReconnectAbandonedEvent;
struct ClientHostLostEvent;
struct WorldSnapshotRequestedEvent;
class EventSystem;

class GameStateManager final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	GameState _state{GameState::Menu};
	GameMode _gameMode{};
	//NOTE: seats, not a count - a seat whose client drops before its ready was never taken
	std::bitset<2> _readySeats{};
	bool _isDemo{};
	bool _isPaused{};
	bool _isScoreBoardShown{};

	void Subscribe();
	void SetState(GameState state);
	void AnnouncePhase() const;
	void Resume();
	[[nodiscard]] GameState IdleStateForMode() const;

	void OnGameModeApplied(const GameModeAppliedEvent& event);
	void OnDemoStarted(const DemoStartedEvent&);
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnGameFinished(const GameFinishedEvent& event);
	void OnMapLoadFailed(const MapLoadFailedEvent&);
	void OnClientReady(const ServerInClientReadyToStartGameEvent& event);
	void OnRestartRequested(const ServerInRestartRequestedEvent&);
	void OnMatchRestartRequested(const MatchRestartRequestedEvent&);
	void OnHostPhase(const HostPhaseAnnouncedEvent& event);
	void OnClientLeft(const ServerInDisconnectEvent& event);
	void OnClientLost(const ServerClientLostEvent& event);
	void OnHostLeft(const ClientInDisconnectEvent&);
	void OnHostUnreachable(const ClientReconnectAbandonedEvent&);
	void OnHostLost(const ClientHostLostEvent&);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;

	void TakeSeat(PlayerSlot slot);
	void FreeSeat(PlayerSlot slot);
	void LoseHost();

	void OnScoreBoardShown(const ScoreBoardShownEvent& event);
	[[nodiscard]] bool IsPauseShown() const;
	void Draw(const PreDrawUserInterfaceEvent&) const;
	void DrawOverScoreBoard(const PostDrawUserInterfaceEvent&) const;
	void Reset(const GameResetEvent&);

public:
	explicit GameStateManager(const std::shared_ptr<EventSystem>& events);

	[[nodiscard]] GameState GetState() const noexcept { return _state; }
};
