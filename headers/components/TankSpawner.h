#pragma once

#include "components/EventSystem.h"
#include "geometry/ObjRectangle.h"
#include "utils/Timer.h"
#include "utils/Uuid.h"
#include <chrono>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

enum class Faction : char8_t;
enum class TankType : char8_t;
enum class GameMode : char8_t;
struct PawnProperty;
struct BonusEffectProperty;
struct GameResetEvent;
struct RespawnTankEvent;
struct SpawnAnimationFinishedEvent;
struct TankRespawnedEvent;
struct TankSpawnCompletedEvent;
struct TankDiedEvent;
struct TankSnapshot;
struct WorldSnapshotRequestedEvent;
struct WorldSnapshotReceivedEvent;
class Tank;
class BaseObj;
class TankPool;
class EventSystem;
class IInputProvider;
class GameConfig;

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
		double speed;
	};

	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
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
	void OnTankDied(const TankDiedEvent& event);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;
	void OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event);

	void Reset(const GameResetEvent&);

	void OnSpawnDelayFinished(Uuid uuid);
	void DelayedSpawnWith(const DelayedTankSpawn& params) const;
	void RestoreTank(const TankSnapshot& tank) const;
	void CancelDelayedSpawnsOf(Faction faction);
	void DropDelayedSpawn(Uuid uuid);

	//NOTE: the rolled point first, then the grid outwards from it - obstacles sit on the grid, so a spot
	//off it that fits covers at least one more cell than the aligned one beside it and finds nothing new
	[[nodiscard]] std::optional<ObjRectangle> FindSpawnSpot(double minX, double maxX, double y,
															double preferredX) const;

	[[nodiscard]] std::optional<ObjRectangle> GetEnemyRandomPosX(TankType type) const;
	[[nodiscard]] bool SpawnEnemy(ObjRectangle rect, Uuid uuid, TankType type, double speed, int health);
	void SpawnPlayer(ObjRectangle rect, double speed, int health, Uuid uuid, TankType type);
	void SpawnCoopBot(ObjRectangle rect, double speed, int health, Uuid uuid, TankType type);

	void DelayedSpawnStart(ObjRectangle rect, int health, double speed, Uuid uuid, TankType type);
	[[nodiscard]] std::unique_ptr<IInputProvider> MakeDriver(TankType type) const;

	void RespawnEnemyTanks(TankType type, Uuid uuid, std::optional<ObjRectangle> rect = std::nullopt);
	[[nodiscard]] std::optional<ObjRectangle> GetPlayerRandomPosX(bool isFirst) const;
	void RespawnPlayerTeam(TankType type, Uuid uuid, std::optional<ObjRectangle> rect = std::nullopt);
	void RespawnTank(TankType type, Uuid uuid, std::optional<ObjRectangle> rect = std::nullopt);

	void OnClientRespawn(TankType type, Uuid uuid, ObjRectangle rect);

public:
	TankSpawner(const GameConfig& gameConfig, const std::vector<std::shared_ptr<BaseObj>>& allObjects,
				const std::shared_ptr<EventSystem>& events, const std::shared_ptr<TankPool>& tankPool);
};
