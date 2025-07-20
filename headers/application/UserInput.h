#pragma once

#include "../Point.h"
#include "../components/input/MouseButton.h"
#include <chrono>

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

	UPoint _windowSize{};
	std::shared_ptr<EventSystem> _events{nullptr};

	std::chrono::system_clock::time_point _lastMoveEventTime{}; //TODO: move to time utils, save to timer item
	milliseconds _moveEndDelay{150};

	void MouseEvents(const SDL_Event& event);
	void KeyPressed(const SDL_Event& event) const;
	void KeyReleased(const SDL_Event& event) const;
	void KeyboardEvents(const SDL_Event& event) const;
	void OnWindowMoveStop();

	void Subscribe();
	void Unsubscribe() const;

	void WindowsMoveEvents(const SDL_Event& event);

public:
	UserInput(UPoint windowSize, std::shared_ptr<EventSystem> events);

	~UserInput();

	void Update();

	[[nodiscard]] bool IsGameOver() const;
	[[nodiscard]] bool IsPause() const;
};
