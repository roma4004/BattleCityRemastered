#include "components/managers/FramePerSecondManager.h"
#include "components/EventSystem.h"
#include <SDL_events.h>

FramePerSecondManager::FramePerSecondManager(const std::shared_ptr<EventSystem>& events, bool isVsyncOn)
	: _name{"FramePerSecondManager"}, _events{events}, _isVsyncOn{isVsyncOn}
{
	_targetFrameDuration = std::chrono::duration<double>{1.0 / static_cast<double>(_targetFps)};

	Subscribe();
}

FramePerSecondManager::~FramePerSecondManager()
{
	if (_frameTimer)
	{
		SDL_RemoveTimer(_frameTimer);
		_frameTimer = 0;
	}

	Unsubscribe();
}

void FramePerSecondManager::Subscribe()
{
	_events->AddListener("CalculateActualFps", _name, [this]()
	{
		_events->EmitEvent("ActualFPS", CountFpsAndDeltaTime());
	});

	_events->AddListener("FrameStart", _name, [this]()
	{
		this->_startFrameTime = std::chrono::high_resolution_clock::now();
	});
}

void FramePerSecondManager::Unsubscribe() const
{
	_events->RemoveListener("CalculateActualFps", _name);
	_events->RemoveListener("FrameStart", _name);
}

static Uint32 FrameTimerCallback(Uint32 /*interval*/, void* param)
{
	const auto frameReady = static_cast<bool*>(param);
	*frameReady = true;

	return 0;
}

unsigned int FramePerSecondManager::CountFpsAndDeltaTime()
{
	static auto lastFpsUpdate = std::chrono::high_resolution_clock::now();
	static unsigned int lastDisplayedFps{0};
	static unsigned int frameCounter{0};

	std::chrono::high_resolution_clock::time_point endFrameTime = std::chrono::high_resolution_clock::now();
	auto frameDuration = std::chrono::duration<double>(endFrameTime - _startFrameTime);
	_deltaTime = static_cast<float>(frameDuration.count());

	if (!_isVsyncOn)
	{
		if (const auto timeToWait = _targetFrameDuration - frameDuration;
			timeToWait.count() > 0)
		{
			_frameReady = false;
			const auto waitMs = static_cast<unsigned int>(timeToWait.count() * 1000.0);
			_frameTimer = SDL_AddTimer(waitMs, FrameTimerCallback, &_frameReady);
			if (waitMs > 5)
			{
				SDL_Delay(waitMs - 5);
			}

			while (!_frameReady)
			{
				SDL_PumpEvents();
			}

			if (_frameTimer)
			{
				SDL_RemoveTimer(_frameTimer);
				_frameTimer = 0;
			}

			endFrameTime = std::chrono::high_resolution_clock::now();
			frameDuration = std::chrono::duration<double>(endFrameTime - _startFrameTime);
			_deltaTime = static_cast<float>(frameDuration.count());
		}
	}

	_events->EmitEvent("DeltaTime", _deltaTime);

	frameCounter++;
	if (const auto timeSinceLastFpsUpdate = std::chrono::duration<double>(endFrameTime - lastFpsUpdate);
		timeSinceLastFpsUpdate.count() >= 1.0)
	{
		const auto fps = static_cast<unsigned int>(std::round(frameCounter / timeSinceLastFpsUpdate.count()));
		frameCounter = 0;
		lastFpsUpdate = endFrameTime;

		if (fps != lastDisplayedFps)
		{
			lastDisplayedFps = std::min(fps, 1000u);
		}
	}

	return lastDisplayedFps;
}
