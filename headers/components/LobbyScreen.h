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

class LobbyScreen final
{
	Point _pos{.x = 25, .y = 25};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	EventSubscription _drawSub{};

	const GameConfig& _gameConfig;

	bool _isLobby{};
	bool _isMenuShown{};
	bool _isServerFull{};

	void Subscribe();
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);
	void OnMenuShowed(const MenuShowedEvent& event);
	void OnRefusedOrLost(const ClientInDisconnectEvent& event);
	void OnConnectedToHost(const ClientConnectedToHostEvent&);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;

	void Display(bool isDisplayed);
	void Draw() const;

public:
	LobbyScreen(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
