#pragma once

#include "components/EventSystem.h"
#include "geometry/ObjRectangle.h"
#include "utils/Timer.h"
#include "utils/Uuid.h"
#include <optional>
#include <random>
#include <vector>

enum class Faction : char8_t;
struct PawnProperty;
enum class TankType : char8_t;
enum class GameMode : char8_t;
struct BonusEffectProperty;
class Tank;
class BaseObj;
class BulletPool;
class EventSystem;
class IInputProvider;
class GameConfig;
struct GameResetEvent;
struct RespawnTankEvent;
struct SpawnAnimationFinishedEvent;
struct TankRespawnedEvent;
struct TankSpawnCompletedEvent;

class TankSpawner final
{
	using milliseconds = std::chrono::milliseconds;
	// Stashed while the spawn animation plays; Tank is constructed once the delay finishes.
	struct DelayedTankSpawn
	{
		Uuid uuid;
		TankType type;
		ObjRectangle rect;
		int health;
		std::string name;
		Faction faction{};
		float speed;
	};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::vector<EventSubscription> _subs{};
	Timer _enemySpawnTimer{};
	GameMode _gameMode{};
	const GameConfig& _gameConfig;
	std::vector<DelayedTankSpawn> _delayedSpawns{};

	void Subscribe();
	void OnRespawnTank(const RespawnTankEvent& event);
	void OnSpawnAnimationFinished(const SpawnAnimationFinishedEvent& event);
	void OnTankRespawned(const TankRespawnedEvent& event);
	void OnTankSpawnCompleted(const TankSpawnCompletedEvent& event);

	void Reset(const GameResetEvent&);

	void OnSpawnDelayFinished(Uuid uuid);
	void DelayedSpawnWith(const DelayedTankSpawn& params);

	[[nodiscard]] ObjRectangle GetEnemyRandomPosX(TankType type) const;
	[[nodiscard]] bool SpawnEnemy(ObjRectangle rect, Uuid uuid, TankType type, float speed, int health);
	void SpawnPlayer(ObjRectangle rect, float speed, int health, Uuid uuid, TankType type);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, Uuid uuid, TankType type);

	void DelayedSpawnStart(ObjRectangle rect, int health, const std::string& name, Faction faction, float speed,
						   Uuid uuid, TankType type);
	[[nodiscard]] std::unique_ptr<IInputProvider> GetInputProvider(TankType type) const;
	[[nodiscard]] std::shared_ptr<Tank> CreateTank(TankType type, PawnProperty pawnProperty);

	void RespawnEnemyTanks(TankType type, Uuid uuid, std::optional<ObjRectangle> rect = std::nullopt);
	[[nodiscard]] ObjRectangle GetPlayerRandomPosX(bool isFirst) const;
	void RespawnPlayerTeam(TankType type, Uuid uuid, std::optional<ObjRectangle> rect = std::nullopt);
	void RespawnTank(TankType type, Uuid uuid, std::optional<ObjRectangle> rect = std::nullopt);

	void OnClientRespawn(TankType type, Uuid uuid, ObjRectangle rect);

public:
	TankSpawner(const GameConfig& gameConfig, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				const std::shared_ptr<EventSystem>& events);

	~TankSpawner() = default;
};
