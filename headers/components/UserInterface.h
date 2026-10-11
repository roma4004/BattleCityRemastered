#pragma once

#include <memory>

class EventSystem;
class FramePerSecondManager;
class GameStateManager;
class LobbyScreen;
class Menu;
class RightSideBar;
class ScoreBoard;
class ServerScreen;

//NOTE: the interface over the field, in an order named here - a hidden part is called all the same and draws nothing
class UserInterface final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	const GameStateManager& _gameState;
	const ScoreBoard& _scoreBoard;
	const RightSideBar& _rightSideBar;
	const LobbyScreen& _lobbyScreen;
	const ServerScreen& _serverScreen;
	Menu& _menu;
	const FramePerSecondManager& _fps;

	void DrawPause() const;

public:
	UserInterface(const std::shared_ptr<EventSystem>& events, const GameStateManager& gameState,
				  const ScoreBoard& scoreBoard, const RightSideBar& rightSideBar, const LobbyScreen& lobbyScreen,
				  const ServerScreen& serverScreen, Menu& menu, const FramePerSecondManager& fps);

	void Draw() const;
};
