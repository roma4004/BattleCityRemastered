#pragma once

#include "utils/Timer.h"

class Tank;
class EventSystem;

class SpawnDelayManager
{
	using milliseconds = std::chrono::milliseconds;

	struct SpawnDelay
	{
		std::shared_ptr<Tank> tank{};
		Timer timer;
	};

	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<SpawnDelay> _spawnDelays{};

public:
	explicit SpawnDelayManager(const std::shared_ptr<EventSystem>& events);

	~SpawnDelayManager();

	void Subscribe();
	void Unsubscribe() const;

	void Reset();

	void TickUpdate(float);
	void Disposer();

};
