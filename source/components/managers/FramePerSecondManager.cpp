#include "components/managers/FramePerSecondManager.h"
#include "components/EventSystem.h"
#include <cmath>//NOTE: need for cmake build
#include <thread>

FramePerSecondManager::FramePerSecondManager(const std::shared_ptr<EventSystem>& events, const bool isVsyncOn)
	: _name{"FramePerSecondManager"}
	, _events{events}
	, _isVsyncOn{isVsyncOn}
{
	_targetFrameDuration = std::chrono::duration<double>{1.0 / static_cast<double>(_targetFps)};

	Subscribe();
}

FramePerSecondManager::~FramePerSecondManager()
{
	Unsubscribe();
}

void FramePerSecondManager::Subscribe()
{
	_events->AddListener("CalculateActualFps", _name, [this]() { this->CountFpsAndDeltaTime(); });

	_events->AddListener("FrameStart", _name, [this]()
	{
		this->_startFrameTime = std::chrono::high_resolution_clock::now();
	});

	_events->AddListener("PostDrawUserInterface", _name, [this]()
	{
		this->_events->EmitEvent("RenderFPS", _lastDisplayedFps);
	});
}

void FramePerSecondManager::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void FramePerSecondManager::CountFpsAndDeltaTime()
{
	if (!_isVsyncOn)
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

			while ((std::chrono::high_resolution_clock::now() - _startFrameTime) < _targetFrameDuration)
			{
				std::this_thread::yield();
			}
		}
	}

	_deltaTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - _startFrameTime).count();
	_events->EmitEvent("DeltaTime", _deltaTime);

	_frameCounter++;
	_fpsAccumulatedTime += _deltaTime;

	if (_fpsAccumulatedTime >= 1.0)
	{
		_lastDisplayedFps = std::min(
				static_cast<unsigned int>(std::round(static_cast<double>(_frameCounter) / _fpsAccumulatedTime)), 1000u);
		_frameCounter = 0u;
		_fpsAccumulatedTime = 0.0;
	}
}
