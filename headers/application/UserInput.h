#pragma once

#include "../Point.h"
#include "../components/input/MouseButton.h"
#include "SDLEnvironment.h"
#include "components/input/InputProviderForMenu.h"

#include <chrono>
#include <map>

union SDL_Event;
class EventSystem;

class UserInput final
{
	using milliseconds = std::chrono::milliseconds;

	MouseButtons _mouseButtons{};
	bool _isGameOver{false};
	bool _isPause{false};
	bool _isPauseBeforeDragNDrop{false};
	bool _isMoving{false};
	std::string _name{"UserInput"};
	bool _areControllersSwapped{false};
	UPoint _windowSize{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::chrono::system_clock::time_point _lastMoveEventTime{};//TODO: move to time utils, save to timer item
	milliseconds _moveEndDelay{150};
	int deviceIndex{0};
	std::map<SDL_JoystickID, SDL_GameController*> controllers;

	void MouseEvents(const SDL_Event& event);
	void KeyPressed(const SDL_Event& event) const;
	void KeyReleased(const SDL_Event& event) const;
	void KeyboardEvents(const SDL_Event& event) const;
	void GamepadKeyPressed(const SDL_Event& event) const;
	void GamepadKeyReleased(const SDL_Event& event) const;
	void GamepadEvents(const SDL_Event& event) const;
	void GamepadsPlugAndPlay(const SDL_Event& event);
	void OnWindowMoveStop();
	void Subscribe();
	void Unsubscribe() const;
	void WindowsMoveEvents(const SDL_Event& event);
	void GamepadInit(SDL_Event& event);
	void SwapControllers();
	std::string ControllerTagDefiner(const SDL_Event& event) const;

public:
	UserInput(UPoint windowSize, std::shared_ptr<EventSystem> events);
	~UserInput();
	static int GetDeviceIndex(const SDL_Event& event);
	bool _removedID{false};
	void Update();

	[[nodiscard]] bool IsGameOver() const;
	[[nodiscard]] bool IsPause() const;
};
