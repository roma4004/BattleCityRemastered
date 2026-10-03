#pragma once

#include "components/EventSystem.h"
#include "geometry/ObjRectangle.h"
#include "utils/Timer.h"
#include "utils/Uuid.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

enum class Author : char8_t;
enum class Faction : char8_t;
enum class TankModel : char8_t;
enum class TankType : char8_t;
enum class GameMode : char8_t;
enum class PlayerSlot : std::uint8_t;
struct PawnProperty;
struct BonusEffectProperty;
struct EnemyLineupLoadedEvent;
struct GameResetEvent;
struct NextLevelRequestedEvent;
struct PostTickUpdateEvent;
struct RespawnTankEvent;
struct SpawnAnimationFinishedEvent;
struct TankRespawnedEvent;
struct TankSpawnCompletedEvent;
struct TankSpawnMovedEvent;
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
		TankModel model{};
		ObjRectangle rect;
		//NOTE: the square it was rolled for - the burst goes back to it the moment it is free again
		ObjRectangle home{};
		int health;
		unsigned short tier{1u};
	};

	//NOTE: what a player takes to the next level, read while the field it was won on still stands and
	//spent on its first spawn there - dying inside a level costs the tier as it always did
	struct NextLevelLoadout
	{
		TankType type{};
		unsigned short tier{1u};
		int health{};
		bool isShipActive{};
	};

	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::vector<EventSubscription> _subs{};
	Timer _enemySpawnTimer{};
	GameMode _gameMode{};
	const GameConfig& _gameConfig;
	std::vector<DelayedTankSpawn> _delayedSpawns{};
	std::vector<NextLevelLoadout> _nextLevelLoadouts{};
	//NOTE: the models of the map's first enemies and how far down them the enemies have come - past, they are rolled
	std::vector<TankModel> _enemyLineup{};
	std::size_t _enemiesChosen{};

	void Subscribe();
	void OnRespawnTank(const RespawnTankEvent& event);
	void OnNextLevelRequested(const NextLevelRequestedEvent&);
	void OnEnemyLineupLoaded(const EnemyLineupLoadedEvent& event);
	void OnSpawnAnimationFinished(const SpawnAnimationFinishedEvent& event);
	void OnTankRespawned(const TankRespawnedEvent& event);
	void OnTankSpawnCompleted(const TankSpawnCompletedEvent& event);
	void OnTankSpawnMoved(const TankSpawnMovedEvent& event);
	void OnPostTickUpdate(const PostTickUpdateEvent&);
	void OnTankDied(const TankDiedEvent& event);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;
	void OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event);

	void Reset(const GameResetEvent& event);

	[[nodiscard]] NextLevelLoadout LoadoutOf(TankType type) const;
	void SpendLoadout(Uuid uuid, TankType type, int health);

	void OnSpawnDelayFinished(Uuid uuid);

	//NOTE: a burst puts nothing in the world, so the bursts are asked too; the ignored uuid is the asker's own
	[[nodiscard]] bool IsSpawnSpotFree(const ObjRectangle& rect, Uuid ignoredSpawn = {}) const;

	//NOTE: a tank driving into a burst shoves its square aside until home is free - the landing square may move
	void NudgeSpawnSquare(DelayedTankSpawn& spawn);
	void MoveSpawnSquare(DelayedTankSpawn& spawn, const ObjRectangle& to);
	[[nodiscard]] std::shared_ptr<Tank> TankStandingIn(const ObjRectangle& rect) const;
	//NOTE: the way the hull drives first, so the square reads as pushed along and not as jumped
	[[nodiscard]] std::optional<ObjRectangle> RoomOutOfTheWay(const DelayedTankSpawn& spawn,
															  const Tank& pusher) const;
	void DelayedSpawnWith(const DelayedTankSpawn& params);
	void RestoreTank(const TankSnapshot& tank);
	void CancelDelayedSpawnsOf(Faction faction, Author spared);
	void DropDelayedSpawn(Uuid uuid);

	//NOTE: the rolled point first, then the grid outwards from it - obstacles sit on the grid, so a spot
	//off it that fits covers at least one more cell than the aligned one beside it and finds nothing new
	[[nodiscard]] std::optional<ObjRectangle> FindSpawnSpot(double minX, double maxX, double y,
															double preferredX) const;

	[[nodiscard]] std::optional<ObjRectangle> GetEnemyRandomPosX(TankType type) const;
	[[nodiscard]] bool SpawnEnemy(ObjRectangle rect, Uuid uuid, TankType type, TankModel model);
	void SpawnPlayer(ObjRectangle rect, Uuid uuid, TankType type);

	void DelayedSpawnStart(ObjRectangle rect, Uuid uuid, TankType type, TankModel model);
	[[nodiscard]] std::unique_ptr<IInputProvider> MakeDriver(TankType type) const;

	[[nodiscard]] TankModel NextEnemyModel();
	//NOTE: an empty model is the lineup's next - it arrives filled only where the authority already chose it
	void RespawnEnemyTanks(TankType type, Uuid uuid, std::optional<ObjRectangle> rect = std::nullopt,
						   std::optional<TankModel> model = std::nullopt);
	[[nodiscard]] std::optional<ObjRectangle> GetPlayerRandomPosX(PlayerSlot slot) const;
	void RespawnPlayerTeam(TankType type, Uuid uuid, std::optional<ObjRectangle> rect = std::nullopt);
	void RespawnTank(TankType type, Uuid uuid, std::optional<ObjRectangle> rect = std::nullopt,
					 std::optional<TankModel> model = std::nullopt);

	void OnClientRespawn(TankType type, Uuid uuid, ObjRectangle rect, TankModel model);

public:
	TankSpawner(const GameConfig& gameConfig, const std::vector<std::shared_ptr<BaseObj>>& allObjects,
				const std::shared_ptr<EventSystem>& events, const std::shared_ptr<TankPool>& tankPool);
};
