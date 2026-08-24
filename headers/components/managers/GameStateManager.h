#pragma once

#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include <memory>
#include <vector>

struct PauseStatusEvent;
struct PreDrawUserInterfaceEvent;
struct GameResetEvent;
struct GameModeAppliedEvent;
struct GameFinishedEvent;
struct ServerInClientReadyToStartGameEvent;
struct ClientConnectedToHostEvent;
struct ServerInDisconnectEvent;
struct ServerClientLostEvent;
struct ClientInDisconnectEvent;
struct ClientReconnectAbandonedEvent;
class EventSystem;

class GameStateManager
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	GameState _state{GameState::Menu};
	GameMode _gameMode{};
	bool _hasPeer{false};

	void Subscribe();
	void SetState(GameState state);
	[[nodiscard]] GameState IdleStateForMode() const;

	void OnGameModeApplied(const GameModeAppliedEvent& event);
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnGameFinished(const GameFinishedEvent& event);
	void OnClientReady(const ServerInClientReadyToStartGameEvent&);
	void OnConnectedToHost(const ClientConnectedToHostEvent&);
	void OnClientLeft(const ServerInDisconnectEvent&);
	void OnClientLost(const ServerClientLostEvent&);
	void OnHostLeft(const ClientInDisconnectEvent&);
	void OnHostUnreachable(const ClientReconnectAbandonedEvent&);

	void PeerArrived();
	void PeerGone();

	void Draw(const PreDrawUserInterfaceEvent&) const;
	void Reset(const GameResetEvent&);

public:
	explicit GameStateManager(const std::shared_ptr<EventSystem>& events);

	~GameStateManager() = default;

	[[nodiscard]] GameState GetState() const { return _state; }
};
