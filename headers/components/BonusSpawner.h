#pragma once

#include "components/EventSystem.h"
#include "geometry/ObjRectangle.h"
#include "utils/Timer.h"
#include "utils/Uuid.h"
#include <chrono>
#include <memory>
#include <random>
#include <vector>

enum class GameMode : char8_t;
enum class BonusType : char8_t;
struct UPoint;
class BaseObj;
class EventSystem;
class GameConfig;
struct GameResetEvent;
struct WorldGeometryChangedEvent;
struct SpawnAnimationFinishedEvent;
struct TickUpdateEvent;
struct BonusSpawnedEvent;
struct BonusSpawnCompletedEvent;

class BonusSpawner final
{
	using milliseconds = std::chrono::milliseconds;

	struct PendingSpawn
	{
		ObjRectangle rect{};
		BonusType type{};
		Uuid uuid{};
		bool isSuper{};
	};

	std::shared_ptr<EventSystem> _events{nullptr};

	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;

	std::uniform_int_distribution<> _distSpawnPosY{};
	std::uniform_int_distribution<> _distSpawnPosX{};
	std::uniform_int_distribution<> _distSpawnType{};
	std::uniform_int_distribution<> _distSuperRoll{};
	const GameConfig& _gameConfig;

	Timer _spawnTimer;
	std::vector<PendingSpawn> _pendingSpawns{};
	GameMode _gameMode{};

	std::vector<EventSubscription> _subs{};

	void Subscribe();
	void OnWorldGeometryChanged(const WorldGeometryChangedEvent&);
	void OnBonusSpawned(const BonusSpawnedEvent& event);
	void OnSpawnAnimationFinished(const SpawnAnimationFinishedEvent& event);
	void OnBonusSpawnCompleted(const BonusSpawnCompletedEvent& event);

	void Update(const TickUpdateEvent&);
	void Reset(const GameResetEvent&);

	struct RolledBonus
	{
		BonusType type{};
		bool isSuper{};
	};

	[[nodiscard]] RolledBonus RollBonus();
	void ResetSpawnRanges();
	void Materialize(const PendingSpawn& pending) const;
	bool MaterializePending(Uuid uuid);
	[[nodiscard]] Uuid AnnounceSpawn(ObjRectangle rect, BonusType type, Uuid uuid, bool isSuper) const;

public:
	BonusSpawner(const std::shared_ptr<EventSystem>& events, const std::vector<std::shared_ptr<BaseObj>>& allObjects,
				 const GameConfig& gameConfig);

	~BonusSpawner() = default;

	void SpawnRandomBonus(ObjRectangle rect);

	void SpawnBonus(ObjRectangle rect, BonusType type, Uuid uuid = {}, bool isSuper = false);
};
