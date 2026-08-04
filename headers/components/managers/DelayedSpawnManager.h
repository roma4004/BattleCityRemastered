#pragma once

#include "utils/Timer.h"

class Tank;
class EventSystem;

class DelayedSpawnManager
{
	using milliseconds = std::chrono::milliseconds;

	struct SpawnDelay
	{
		std::shared_ptr<Tank> tank{nullptr};
		Timer timer;
	};

	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<SpawnDelay> _spawnDelays{};

public:
	explicit DelayedSpawnManager(const std::shared_ptr<EventSystem>& events);

	~DelayedSpawnManager();

	void Subscribe();
	void Unsubscribe() const;

	void Reset();

	void PreTickUpdate(double deltaTime);
	void Disposer();
	void SpawnDelayStart(const std::shared_ptr<Tank>& tank, milliseconds delay);
};
