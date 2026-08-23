#pragma once

#include "geometry/Point.h"
#include "../components/input/MouseButton.h"
#include "components/EventSystem.h"
#include "components/input/InputProviderForMenu.h"
#include <SDL_gamecontroller.h>
#include <SDL_rect.h>
#include <chrono>
#include <vector>

union SDL_Event;
class EventSystem;
class GameConfig;
struct PauseStatusEvent;
struct TabReleasedEvent;
struct PreTickUpdateEvent;
struct MenuShowedEvent;
struct MenuPosChangedEvent;

class UserInput final
{
	struct SubTile
	{
		SDL_Rect rect;
		GameMode gameMode;
	};

	using milliseconds = std::chrono::milliseconds;

	MouseButtons _mouseButtons{};
	bool _isShutdown{false};
	bool _isPause{false};
	bool _isPausedByWindowDrag{false};
	bool _isWindowDragging{false};
	bool _isMenuDisplayed{false};
	GameMode _selectedGameMode{};
	bool _areControllersSwapped{false};
	//NOTE: the last size we announced, not a copy of the config - drops a repeat without depending
	//on who listened to the previous one
	UPoint _windowSize{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	std::chrono::steady_clock::time_point _lastDragEventTime{};
	milliseconds _dragEndDelay{150};
	std::vector<std::shared_ptr<SDL_GameController>> _slotsForController{};
	GameConfig& _gameConfig;
	SDL_Rect _menuPos{};
	SDL_Rect _allTilesRect;
	SDL_Rect _allTilesRectDefault;
	SDL_Rect _firstMenuMouseTileDefault;
	std::vector<SubTile> _menuTiles;

	void MouseEvents(const SDL_Event& event);
	void KeyboardKeyPressRelease(const SDL_Event& event, const bool& isPressed) const;
	void KeyboardEvents(const SDL_Event& event) const;
	void GamepadKeyPressRelease(const SDL_Event& event, const bool& isPressed) const;
	void GamepadEvents(const SDL_Event& event);
	void OnWindowDragStop();
	void WindowDragEvents(const SDL_Event& event);
	void OnWindowResized(UPoint newSize);

	void Subscribe();
	void OnPauseStatus(const PauseStatusEvent& event);
	void SwapControllers(const TabReleasedEvent&);
	void OnPreTickUpdate(const PreTickUpdateEvent&);
	void OnMenuShowed(const MenuShowedEvent& event);
	void OnMenuPosChanged(const MenuPosChangedEvent& event);

	void InitControllers();
	void ConnectController(const std::shared_ptr<SDL_GameController>& newController);
	void DisconnectController(SDL_JoystickID instanceId);
	[[nodiscard]] std::string ControllerTagDefiner(SDL_JoystickID instanceId) const;
	[[nodiscard]] static bool IsSameController(const std::shared_ptr<SDL_GameController>& controller,
											   SDL_JoystickID instanceId);
	void InitMouseHoverTiles(Point menuPos);

public:
	UserInput(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);
	~UserInput();

	void Update();

	[[nodiscard]] bool IsShutdown() const;
	[[nodiscard]] bool IsPause() const;
};
