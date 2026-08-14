#pragma once

#include "components/EventSystem.h"
#include "entities/ObjRectangle.h"
#include "utils/Timer.h"
#include <optional>
#include <random>
#include <vector>
#include <boost/uuid/uuid.hpp>

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
struct GameModeChangedToEvent;
struct RespawnTankEvent;
struct TankSpawnDelayFinishedEvent;
struct WindowSizeChangedToEvent;
struct ClientInRespawnTankEvent;
struct ClientInTankSpawnCompleteEvent;

class TankSpawner final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	// Stashed while the spawn animation plays; Tank is constructed once the delay finishes.
	struct DelayedTankSpawn
	{
		buuid uuid;
		TankType type;
		ObjRectangle rect;
		int health;
		std::string name;
		std::string fraction;
		float speed;
	};

	std::string _name{"TankSpawner"};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime on every GameModeChangedToEvent, independent of _subs's fixed
	// subscribe-once-at-construction lifetime - assigning a new EventSubscription here
	// auto-unsubscribes whatever was previously held.
	EventSubscription _clientRespawnSub{};
	EventSubscription _clientMaterializeSub{};
	Timer _enemySpawnTimer{};
	GameMode _gameMode{};
	GameConfig& _gameConfig;
	std::vector<DelayedTankSpawn> _delayedSpawns{};

	void Subscribe();
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnRespawnTank(const RespawnTankEvent& event);
	void OnTankSpawnDelayFinished(const TankSpawnDelayFinishedEvent& event);
	void OnWindowSizeChangedTo(const WindowSizeChangedToEvent& event) const;
	void SubscribeAsClient();
	void OnClientInRespawnTank(const ClientInRespawnTankEvent& event);
	void OnClientInTankSpawnComplete(const ClientInTankSpawnCompleteEvent& event);

	void UnsubscribeAsClient();
	void Reset(const GameResetEvent&);

	void OnSpawnDelayFinished(buuid uuid);
	void DelayedSpawnWith(const DelayedTankSpawn& params);

	[[nodiscard]] ObjRectangle GetEnemyRandomPosX(TankType type) const;
	[[nodiscard]] bool SpawnEnemy(ObjRectangle rect, buuid uuid, TankType type, float speed, int health,
								  bool skipDelay = false);
	void SpawnPlayer(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);

	void DelayedSpawnStart(ObjRectangle rect, int health, const std::string& name, std::string fraction, float speed,
						   buuid uuid, TankType type, bool skipDelay = false);
	[[nodiscard]] std::unique_ptr<IInputProvider> GetInputProvider(TankType type) const;
	[[nodiscard]] std::shared_ptr<Tank> CreateTank(TankType type, PawnProperty pawnProperty);

	void RespawnEnemyTanks(TankType type, buuid uuid, bool skipDelay = false,
						   std::optional<ObjRectangle> rect = std::nullopt);
	[[nodiscard]] ObjRectangle GetPlayerRandomPosX(bool isFirst) const;
	void RespawnPlayerTeam(TankType type, buuid uuid, bool skipDelay = false,
						   std::optional<ObjRectangle> rect = std::nullopt);
	void RespawnTank(TankType type, buuid uuid, bool skipDelay, std::optional<ObjRectangle> rect = std::nullopt);
	[[nodiscard]] static std::string GetCurrentTimeString();

	void OnClientRespawn(TankType type, buuid uuid, ObjRectangle rect);

public:
	TankSpawner(GameConfig& gameConfig, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				const std::shared_ptr<EventSystem>& events);

	~TankSpawner() = default;
};
