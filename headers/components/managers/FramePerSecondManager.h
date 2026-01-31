#pragma once

#include <chrono>
#include <SDL_timer.h>

class Tank;
class EventSystem;

class FramePerSecondManager
{
	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};

	std::chrono::duration<double> _targetFrameDuration{};
	SDL_TimerID _frameTimer{};
	const int _targetFps{60};
	bool _frameReady{true};
	bool _isVsyncOn{false};//TODO: add settings inGame for tweak this in real time via subscribe
	std::chrono::time_point<std::chrono::steady_clock> _startFrameTime{};
	float _deltaTime{};

public:
	explicit FramePerSecondManager(const std::shared_ptr<EventSystem>& events, bool isVsyncOn);

	~FramePerSecondManager();

	void Subscribe();
	void Unsubscribe() const;

	[[nodiscard]] unsigned int CountFpsAndDeltaTime();
};
