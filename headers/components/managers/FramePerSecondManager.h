#pragma once

#include <chrono>

class EventSystem;

class FramePerSecondManager
{
	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};

	std::chrono::duration<double> _targetFrameDuration{};
	std::chrono::high_resolution_clock::time_point _startFrameTime{};
	float _deltaTime{0.f};
	float _fpsAccumulatedTime;
	unsigned int _targetFps{60};
	unsigned int _frameCounter{0};
	unsigned int _lastDisplayedFps{0};
	bool _isVsyncOn{false};//TODO: add settings inGame for tweak this in real time via subscribe

	void Subscribe();
	void Unsubscribe() const;

	void CountFpsAndDeltaTime();

public:
	FramePerSecondManager(const std::shared_ptr<EventSystem>& events, bool isVsyncOn);

	~FramePerSecondManager();
};
