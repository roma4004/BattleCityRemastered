#pragma once

#include <chrono>
#include <boost/property_tree/ptree_fwd.hpp>

class EventSystem;

class FramePerSecondManager
{
	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};

	std::chrono::duration<double> _targetFrameDuration{};
	std::chrono::high_resolution_clock::time_point _startFrameTime{};
	double _deltaTime{};
	double _fpsAccumulatedTime{};
	unsigned int _targetFps{60};
	unsigned int _frameCounter{};
	unsigned int _lastDisplayedFps{};
	boost::property_tree::ptree& _pTreeIni;

	void Subscribe();
	void Unsubscribe() const;

	void CountFpsAndDeltaTime();

public:
	FramePerSecondManager(const std::shared_ptr<EventSystem>& events, boost::property_tree::ptree& pTreeIni);

	~FramePerSecondManager();
};
