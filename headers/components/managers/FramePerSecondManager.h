#pragma once

#include "components/EventSystem.h"
#include <chrono>
#include <memory>
#include <vector>

struct CalculateActualFpsEvent;
struct FrameStartEvent;
struct PostDrawUserInterfaceEvent;
class EventSystem;
class ProjectConfig;

class FramePerSecondManager final
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
	//NOTE: without a presenter nothing else blocks this thread, so the wait below is the only
	//thing between the loop and a busy spin - the vsync setting says nothing on a console server
	bool _isVsyncAvailable{true};

	void Subscribe();
	void OnFrameStart(const FrameStartEvent&);
	void OnPostDrawUserInterface(const PostDrawUserInterfaceEvent&) const;

	void CountFpsAndDeltaTime(const CalculateActualFpsEvent&);

public:
	FramePerSecondManager(const std::shared_ptr<EventSystem>& events, const ProjectConfig& projectConfig,
						  bool isVsyncAvailable);
};
