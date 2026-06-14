#pragma once

#include "Point.h"
#include "managers/RespawnManager.h"
#include "utils/Timer.h"
#include <random>
#include <boost/uuid/uuid.hpp>

struct PawnProperty;
enum class TankType : char8_t;
enum class GameMode : char8_t;
struct ObjRectangle;
struct BonusEffectProperty;
class Tank;
class BaseObj;
class BulletPool;
class EventSystem;
class BonusEffectManager;
class IInputProvider;

class TankSpawner final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::string _name{"TankSpawner"};
	UPoint _windowSize{};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	Timer _enemySpawnTimer{};
	GameMode _gameMode{};

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;
	void Reset();

	bool SpawnEnemy(buuid uuid, TankType type, float speed, int health, bool skipDelay = false);
	bool SpawnPlayer(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);

	void SpawnTank(ObjRectangle rect, int health, const std::string& name, std::string fraction, float speed,
				   buuid uuid, TankType type, bool skipDelay = false);
	[[nodiscard]] std::unique_ptr<IInputProvider> GetInputProvider(TankType type);
	[[nodiscard]] std::shared_ptr<Tank> CreateTank(TankType type, PawnProperty pawnProperty);

	void RespawnEnemyTanks(TankType type, buuid uuid, bool skipDelay = false);
	void RespawnPlayerTeam(TankType type, buuid uuid, bool skipDelay = false);
	[[nodiscard]] static std::string GetCurrentTimeString();

	void OnClientRespawn(TankType type, buuid uuid);

public:
	TankSpawner(UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				const std::shared_ptr<EventSystem>& events);

	~TankSpawner();

	void RespawnTanks(bool skipDelay = false);//TODO: still public for unit test
	[[nodiscard]] int GetEnemyRespawnCount() const;//TODO: still public for unit test
	[[nodiscard]] int GetPlayerOneRespawnCount() const;//TODO: still public for unit test
	[[nodiscard]] int GetPlayerTwoRespawnCount() const;//TODO: still public for unit test
};
