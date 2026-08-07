#pragma once

#include "utils/Timer.h"
#include <boost/uuid/uuid.hpp>

class BaseObj;
class EventSystem;

class DelayedSpawnManager
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	struct SpawnDelay
	{
		buuid uuid;
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
	void SpawnDelayStart(const buuid& uuid, milliseconds delay);
};
