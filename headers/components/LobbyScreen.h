#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"
#include <memory>
#include <vector>

struct DrawUserInterfaceEvent;
struct GameStateChangedToEvent;
struct ClientInDisconnectEvent;
struct ClientConnectedToHostEvent;
struct MenuShowedEvent;
class GameConfig;
class EventSystem;

//NOTE: a sibling of Menu and ScoreBoard - waiting for a peer is its own phase, not a menu mode
class LobbyScreen final
{
	Point _pos{.x = 25, .y = 25};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	EventSubscription _drawSub{};

	const GameConfig& _gameConfig;

	bool _isLobby{false};
	bool _isMenuShown{false};
	bool _isServerFull{false};

	void Subscribe();
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);
	void OnMenuShowed(const MenuShowedEvent& event);
	void OnRefusedOrLost(const ClientInDisconnectEvent& event);
	void OnConnectedToHost(const ClientConnectedToHostEvent&);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&);

	void Display(bool isDisplayed);
	void Draw() const;

public:
	LobbyScreen(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
