#pragma once

#include "components/EventSystem.h"
#include <chrono>

class EventSystem;
class GameConfig;
struct CalculateActualFpsEvent;
struct FrameStartEvent;
struct PostDrawUserInterfaceEvent;

class FramePerSecondManager
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	std::chrono::duration<double> _targetFrameDuration{};
	std::chrono::high_resolution_clock::time_point _startFrameTime{};
	double _deltaTime{};
	double _fpsAccumulatedTime{};
	unsigned int _targetFps{60u};
	unsigned int _frameCounter{};
	unsigned int _lastDisplayedFps{};
	GameConfig& _gameConfig;

	void Subscribe();
	void OnFrameStart(const FrameStartEvent&);
	void OnPostDrawUserInterface(const PostDrawUserInterfaceEvent&) const;

	void CountFpsAndDeltaTime(const CalculateActualFpsEvent&);

public:
	FramePerSecondManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~FramePerSecondManager() = default;
};
