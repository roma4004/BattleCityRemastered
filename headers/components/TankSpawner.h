#pragma once

#include "Point.h"
#include <boost/uuid/uuid.hpp>

struct PawnProperty;
enum class TankType : char8_t;
enum class GameMode : char8_t;
struct SDL_Renderer;
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
	std::shared_ptr<BonusEffectManager> _bonusEffectManager{nullptr};


	// TODO: use std::atomic when multithreading is used
	std::vector<int> _respawnResource{20, 3, 3};
	int _enemyNeedRespawn{4};

	struct SpawnSlot
	{
		buuid uuid{};
		bool isAvailable{false};
	};

	GameMode _gameMode{};
	std::vector<SpawnSlot> _slots{};

	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusTank(const std::string& author, const std::string& fraction);

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void SpawnEnemy(buuid uuid, TankType type, float speed, int health, bool skipDelay = false);
	void SetEnemyNeedRespawn();

	void SpawnPlayer(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);

	void SpawnTank(ObjRectangle rect, int color, int health, const std::string& name, const std::string& fraction,
	               float speed, buuid uuid, BonusEffectProperty effects, TankType type, bool skipDelay = false);
	[[nodiscard]] std::unique_ptr<IInputProvider> GetInputProvider(TankType type);
	[[nodiscard]] std::shared_ptr<Tank> CreateTank(TankType type, PawnProperty pawnProperty,
	                                               BonusEffectProperty effects);

	void RespawnEnemyTanks(TankType type, buuid uuid, bool skipDelay = false);
	void RespawnPlayerTeam(TankType type, buuid uuid, bool skipDelay = false);
	void SetPlayerNeedRespawn();
	[[nodiscard]] static std::string GetCurrentTimeString();

	void ResetRespawnStat();
	void OnClientRespawn(TankType type, buuid uuid, bool skipDelay = false);
	void ResetSpawn();

	void IncreaseEnemyRespawnResource();
	void IncreasePlayerOneRespawnResource();
	void IncreasePlayerTwoRespawnResource();

	void DecreaseEnemyRespawnResource();
	void DecreasePlayerOneRespawnResource();
	void DecreasePlayerTwoRespawnResource();
	void OnTankSpawn(const buuid& uuid);
	void OnTankDied(const buuid& uuid);

public:
	TankSpawner(UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool,
	            std::shared_ptr<BonusEffectManager> bonusEffectManager);

	~TankSpawner();

	void RespawnTanks(bool skipDelay = false);

	// NOTE: for unit tests only:
	[[nodiscard]] int GetEnemyRespawnResource() const;
	[[nodiscard]] int GetPlayerOneRespawnResource() const;
	[[nodiscard]] int GetPlayerTwoRespawnResource() const;

	void SetSlotNeedRespawn(int slotIndex);
};
