#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"
#include <memory>
#include <vector>

class GameConfig;
class EventSystem;
struct DrawUserInterfaceEvent;
struct GameStateChangedToEvent;
struct MenuShowedEvent;

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

	void Subscribe();
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);
	void OnMenuShowed(const MenuShowedEvent& event);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&);

	void Display(bool isDisplayed);
	void Draw() const;

public:
	LobbyScreen(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	~LobbyScreen() = default;
};
