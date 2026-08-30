#pragma once

#include "components/EventSystem.h"
#include <chrono>
#include <memory>
#include <vector>

class EventSystem;
class ProjectConfig;
struct CalculateActualFpsEvent;
struct FrameStartEvent;
struct PostDrawUserInterfaceEvent;

class FramePerSecondManager
{
	static constexpr unsigned int kTargetFps{60u};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	std::chrono::duration<double> _targetFrameDuration{};
	std::chrono::high_resolution_clock::time_point _startFrameTime{};
	double _deltaTime{};
	double _fpsAccumulatedTime{};

	unsigned int _frameCounter{};
	unsigned int _lastDisplayedFps{};
	const ProjectConfig& _projectConfig;

	void Subscribe();
	void OnFrameStart(const FrameStartEvent&);
	void OnPostDrawUserInterface(const PostDrawUserInterfaceEvent&) const;

	void CountFpsAndDeltaTime(const CalculateActualFpsEvent&);

public:
	FramePerSecondManager(const std::shared_ptr<EventSystem>& events, const ProjectConfig& projectConfig);

	~FramePerSecondManager() = default;
};
