#pragma once

#include "components/EventSystem.h"
#include "components/MatchSettings.h"
#include <memory>
#include <optional>
#include <vector>

struct DrawUserInterfaceEvent;
struct GameStateChangedToEvent;
struct ClientInDisconnectEvent;
struct ClientConnectedToHostEvent;
struct MenuShownEvent;
struct PlayerSlotAssignedEvent;
struct ServerScreenShownEvent;
class GameConfig;
class EventSystem;

class LobbyScreen final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	EventSubscription _drawSub{};

	const GameConfig& _gameConfig;

	bool _isLobby{};
	bool _isMenuShown{};
	bool _isServerScreenShown{};
	bool _isServerFull{};
	//NOTE: what the host picked - every seat is shown it, only the host could change it
	std::optional<MatchSettings> _match{};

	void Subscribe();
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);
	void OnMenuShown(const MenuShownEvent& event);
	void OnServerScreenShown(const ServerScreenShownEvent& event);
	void OnRefusedOrLost(const ClientInDisconnectEvent& event);
	void OnConnectedToHost(const ClientConnectedToHostEvent&);
	void OnSlotAssigned(const PlayerSlotAssignedEvent& event);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;

	void Display();
	void Draw() const;

public:
	LobbyScreen(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
