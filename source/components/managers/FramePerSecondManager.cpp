#include "components/managers/FramePerSecondManager.h"
#include "components/EventSystem.h"
#include <thread>

FramePerSecondManager::FramePerSecondManager(const std::shared_ptr<EventSystem>& events, bool isVsyncOn)
	: _name{"FramePerSecondManager"}, _events{events}, _isVsyncOn{isVsyncOn}
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
	_events->AddListener("CalculateActualFps", _name, [this]()
	{
		this->CountFpsAndDeltaTime();
	});

	_events->AddListener("FrameStart", _name, [this]()
	{
		this->_startFrameTime = std::chrono::high_resolution_clock::now();
	});

	_events->AddListener("PostDrawUserInterface", _name, [this]()
	{
		this->_events->EmitEvent("RenderFPS", _lastDisplayedFps);
	});
}

void FramePerSecondManager::Unsubscribe() const
{
	_events->RemoveListener("CalculateActualFps", _name);
	_events->RemoveListener("FrameStart", _name);
	_events->RemoveListener("PostDrawUserInterface", _name);
}

void FramePerSecondManager::CountFpsAndDeltaTime()
{
	std::chrono::high_resolution_clock::time_point endFrameTime = std::chrono::high_resolution_clock::now();
	auto frameDuration = std::chrono::duration<double>(endFrameTime - _startFrameTime);
	_deltaTime = static_cast<float>(frameDuration.count());

	if (!_isVsyncOn)
	{
		if (const auto timeToWait = _targetFrameDuration - frameDuration;
			timeToWait.count() > 0)
		{
			if (timeToWait.count() > 0.002) { // 2ms
				std::this_thread::sleep_for(timeToWait - std::chrono::milliseconds(1));
			}

			while ((std::chrono::high_resolution_clock::now() - _startFrameTime) < _targetFrameDuration) {
				std::this_thread::yield();
			}

			endFrameTime = std::chrono::high_resolution_clock::now();
			frameDuration = std::chrono::duration<double>(endFrameTime - _startFrameTime);
			_deltaTime = static_cast<float>(frameDuration.count());
		}
	}

	_events->EmitEvent("DeltaTime", _deltaTime);

	_frameCounter++;
	_fpsAccumulatedTime += _deltaTime;

	if (_fpsAccumulatedTime >= 1.0)
	{
		const auto fps = static_cast<unsigned int>(std::round(static_cast<float>(_frameCounter) / _fpsAccumulatedTime));
		_frameCounter = 0;
		_fpsAccumulatedTime = 0.0;

		_lastDisplayedFps = std::min(fps, 1000u);
	}
}
