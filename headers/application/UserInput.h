#pragma once

#include "../Point.h"
#include "../components/input/MouseButton.h"
#include "SDLEnvironment.h"
#include "components/input/InputProviderForMenu.h"
#include <chrono>
#include <vector>

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
	std::chrono::system_clock::time_point _lastMoveEventTime{};
	milliseconds _moveEndDelay{150};
	std::vector<std::shared_ptr<SDL_GameController>> _slotsForController{};

	void MouseEvents(const SDL_Event& event);
	void KeyboardKeyPressRelease(const SDL_Event& event) const;
	void KeyboardEvents(const SDL_Event& event) const;
	void GamepadKeyPressRelease(const SDL_Event& event, const std::string& KeyStateTag) const;
	void GamepadEvents(const SDL_Event& event);
	void OnWindowMoveStop();
	void WindowsMoveEvents(const SDL_Event& event);

	void Subscribe();
	void Unsubscribe() const;

	void InitControllers();
	void ConnectController(const std::shared_ptr<SDL_GameController>& newController);
	void DisconnectController(SDL_JoystickID instanceId);
	void SwapControllers();
	std::string ControllerTagDefiner(const SDL_JoystickID instanceId) const;
	static bool IsSameController(const std::shared_ptr<SDL_GameController>& controller, SDL_JoystickID instanceId);

public:
	UserInput(UPoint windowSize, const std::shared_ptr<EventSystem>& events);
	~UserInput();

	void Update();

	[[nodiscard]] bool IsGameOver() const;
	[[nodiscard]] bool IsPause() const;
};
