#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include "utils/Timer.h"
#include <optional>
#include <random>
#include <boost/uuid/uuid.hpp>

struct PawnProperty;
enum class TankType : char8_t;
enum class GameMode : char8_t;
struct BonusEffectProperty;
class Tank;
class BaseObj;
class BulletPool;
class EventSystem;
class BonusEffectManager;
class IInputProvider;
class GameConfig;
class RespawnManager;

class TankSpawner final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::string _name{"TankSpawner"};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	RespawnManager& _respawnManager;
	Timer _enemySpawnTimer{};
	GameMode _gameMode{};
	GameConfig& _gameConfig;

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;
	void Reset();

	[[nodiscard]] ObjRectangle GetEnemyRandomPosX(TankType type) const;
	[[nodiscard]] bool SpawnEnemy(ObjRectangle rect, buuid uuid, TankType type, float speed, int health,
								  bool skipDelay = false);
	void SpawnPlayer(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);

	void SpawnTank(ObjRectangle rect, int health, const std::string& name, std::string fraction, float speed,
				   buuid uuid, TankType type, bool skipDelay = false);
	[[nodiscard]] std::unique_ptr<IInputProvider> GetInputProvider(TankType type);
	[[nodiscard]] std::shared_ptr<Tank> CreateTank(TankType type, PawnProperty pawnProperty);

	void RespawnEnemyTanks(TankType type, buuid uuid, bool skipDelay = false,
						   std::optional<ObjRectangle> rect = std::nullopt);
	[[nodiscard]] ObjRectangle GetPlayerRandomPosX(bool isFirst) const;
	void RespawnPlayerTeam(TankType type, buuid uuid, bool skipDelay = false,
						   std::optional<ObjRectangle> rect = std::nullopt);
	void RespawnTanks(bool skipDelay);
	void RespawnTank(TankType type, buuid uuid, bool skipDelay, std::optional<ObjRectangle> rect = std::nullopt);
	[[nodiscard]] static std::string GetCurrentTimeString();

	void OnClientRespawn(TankType type, buuid uuid, ObjRectangle rect);

public:
	TankSpawner(GameConfig& gameConfig, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				const std::shared_ptr<EventSystem>& events, RespawnManager& respawnManager);

	~TankSpawner();
};
