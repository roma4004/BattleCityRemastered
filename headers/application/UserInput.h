#pragma once

#include "../MouseButton.h"

#include <SDL.h>
#include <memory>
#include <string>

struct Window;
class EventSystem;

class UserInput final
{
	MouseButtons _mouseButtons{};
	bool _isGameOver{false};
	bool _isPause{false};
	std::string _name{"UserInput"};

	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};

	void MouseEvents(const SDL_Event& event);
	void KeyPressed(const SDL_Event& event) const;
	void KeyReleased(const SDL_Event& event) const;
	void KeyboardEvents(const SDL_Event& event) const;

	void Subscribe();
	void Unsubscribe() const;

public:
	UserInput(std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events);

	~UserInput();

	void Update();
	[[nodiscard]] bool IsGameOver() const;
	[[nodiscard]] bool IsPause() const;
};
