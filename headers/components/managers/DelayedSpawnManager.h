#pragma once

#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include "utils/Timer.h"
#include <boost/uuid/uuid.hpp>

class BaseObj;
class EventSystem;
struct GameResetEvent;
struct SpawnDelayStartEvent;
struct GameModeChangedToEvent;
struct PreTickUpdateEvent;
struct PostTickUpdateEvent;

class DelayedSpawnManager
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	struct SpawnDelay
	{
		buuid uuid;
		Timer timer;
	};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<SpawnDelay> _spawnDelays{};
	std::vector<EventSubscription> _subs{};
	GameMode _gameMode{};

	void OnGameReset(const GameResetEvent&);
	void OnSpawnDelayStart(const SpawnDelayStartEvent& event);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnPreTickUpdate(const PreTickUpdateEvent& event);
	void OnPostTickUpdate(const PostTickUpdateEvent&);

public:
	explicit DelayedSpawnManager(const std::shared_ptr<EventSystem>& events);

	~DelayedSpawnManager() = default;

	void Subscribe();

	void Reset();

	void PreTickUpdate(double deltaTime);
	void Disposer();
	void SpawnDelayStart(const buuid& uuid, milliseconds delay);
};
