#pragma once

#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include "utils/Timer.h"
#include "utils/Uuid.h"

class BaseObj;
class GameConfig;
class EventSystem;
struct GameResetEvent;
struct SpawnDelayStartEvent;
struct PreTickUpdateEvent;
struct PostTickUpdateEvent;

class DelayedSpawnManager
{
	using milliseconds = std::chrono::milliseconds;
	struct SpawnDelay
	{
		Uuid uuid;
		Timer timer;
	};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<SpawnDelay> _spawnDelays{};
	std::vector<EventSubscription> _subs{};
	GameConfig& _gameConfig;

	void OnGameReset(const GameResetEvent&);
	void OnSpawnDelayStart(const SpawnDelayStartEvent& event);
	void OnPreTickUpdate(const PreTickUpdateEvent& event);
	void OnPostTickUpdate(const PostTickUpdateEvent&);

public:
	DelayedSpawnManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~DelayedSpawnManager() = default;

	void Subscribe();

	void Reset();

	void PreTickUpdate(double deltaTime);
	void Disposer();
	void SpawnDelayStart(const Uuid& uuid, milliseconds delay);
};
