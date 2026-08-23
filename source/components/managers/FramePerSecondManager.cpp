#include "components/managers/FramePerSecondManager.h"
#include "application/ProjectConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include <cmath>//NOTE: need for cmake build
#include <thread>

FramePerSecondManager::FramePerSecondManager(const std::shared_ptr<EventSystem>& events,
											 const ProjectConfig& projectConfig)
	: _events{events}
	, _projectConfig{projectConfig}
{
	_targetFrameDuration = std::chrono::duration<double>{1.0 / static_cast<double>(_targetFps)};

	Subscribe();
}

void FramePerSecondManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &FramePerSecondManager::CountFpsAndDeltaTime));
	_subs.push_back(_events->AddListener(this, &FramePerSecondManager::OnFrameStart));
	_subs.push_back(_events->AddListener(this, &FramePerSecondManager::OnPostDrawUserInterface));
}

void FramePerSecondManager::OnFrameStart(const FrameStartEvent&)
{
	_startFrameTime = std::chrono::high_resolution_clock::now();
}

void FramePerSecondManager::OnPostDrawUserInterface(const PostDrawUserInterfaceEvent&) const
{
	_events->EmitEvent(RenderFPSEvent{.fps = _lastDisplayedFps});
}

void FramePerSecondManager::CountFpsAndDeltaTime(const CalculateActualFpsEvent&)
{
	if (!_projectConfig.IsVsyncOn())
	{
		const auto currentFrameDuration = std::chrono::duration<double>(
				std::chrono::high_resolution_clock::now() - _startFrameTime);
		if (const auto timeToWait = _targetFrameDuration - currentFrameDuration;
			timeToWait.count() > 0)
		{
			if (timeToWait.count() > 0.002)
			{
				std::this_thread::sleep_for(timeToWait - std::chrono::milliseconds(1));
			}

			while (std::chrono::high_resolution_clock::now() - _startFrameTime < _targetFrameDuration)
			{
				std::this_thread::yield();
			}
		}
	}

	const double measuredFrameTime =
			std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - _startFrameTime).count();

	//NOTE: this long means the loop stalled - window drag, resize, a breakpoint - not slow hardware.
	//Movement is speed * deltaTime, so the measured value would teleport everything in one step; give
	//it one ordinary frame instead. Threshold is tankSize / tankSpeed (36/142) - one tank length.
	constexpr double kHitchThreshold{0.25};
	_deltaTime = measuredFrameTime > kHitchThreshold ? _targetFrameDuration.count() : measuredFrameTime;
	_events->EmitEvent(DeltaTimeEvent{.deltaTime = _deltaTime});

	_frameCounter++;
	//NOTE: the real duration - the fps window has to stay one real second
	_fpsAccumulatedTime += measuredFrameTime;

	if (_fpsAccumulatedTime >= 1.0)
	{
		_lastDisplayedFps = std::min(
				static_cast<unsigned int>(std::round(static_cast<double>(_frameCounter) / _fpsAccumulatedTime)), 1000u);
		_frameCounter = 0u;
		_fpsAccumulatedTime = 0.0;
	}
}
