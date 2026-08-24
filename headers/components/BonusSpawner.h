#pragma once

#include "components/EventSystem.h"
#include "utils/Timer.h"
#include "utils/Uuid.h"
#include <random>
#include <vector>

enum class GameMode : char8_t;
enum class BonusType : char8_t;
struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;
class GameConfig;
struct GameResetEvent;
struct WorldGeometryChangedEvent;
struct TickUpdateEvent;
struct BonusSpawnedEvent;

class BonusSpawner final
{
	using milliseconds = std::chrono::milliseconds;
	std::shared_ptr<EventSystem> _events{nullptr};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::uniform_int_distribution<> _distSpawnPosY{};
	std::uniform_int_distribution<> _distSpawnPosX{};
	std::uniform_int_distribution<> _distSpawnType{};
	const GameConfig& _gameConfig;

	Timer _spawnTimer;
	GameMode _gameMode{};

	std::vector<EventSubscription> _subs{};

	void Subscribe();
	void OnWorldGeometryChanged(const WorldGeometryChangedEvent&);
	void OnBonusSpawned(const BonusSpawnedEvent& event);

	void Update(const TickUpdateEvent&);
	void Reset(const GameResetEvent&);

public:
	BonusSpawner(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				 const GameConfig& gameConfig);

	~BonusSpawner() = default;

	void SpawnRandomBonus(ObjRectangle rect);

	void SpawnBonus(ObjRectangle rect, BonusType type, Uuid uuid = {});//NOTE: for unit tests
};
