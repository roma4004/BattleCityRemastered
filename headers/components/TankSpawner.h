#pragma once

#include "../Point.h"
#include "../enums/GameMode.h"
#include "../enums/RespawnResource.h"
#include <memory>
#include <random>
#include <boost/uuid/uuid.hpp>

struct PawnProperty;
enum TankType : char8_t;
enum GameMode : char8_t;
struct SDL_Renderer;
struct ObjRectangle;
struct BonusEffectProperty;
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
	GameMode _gameMode{};
	UPoint _windowSize{};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<BonusEffectManager> _bonusEffectManager;

	std::random_device _rd;

	// TODO: use std::atomic when multithreading is used
	std::vector<int> _respawnResource{20, 3, 3};
	int _enemyNeedRespawn{4};

	struct SpawnSlot
	{
		buuid id{};
		bool isAvailable{false};
	};

	std::vector<SpawnSlot> _slots;

	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusTank(const std::string& author, const std::string& fraction);

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void SpawnEnemy(buuid uuid, TankType type, float speed, int health);
	void SetEnemyNeedRespawn();

	void SpawnPlayer(ObjRectangle rect, float speed, int health, buuid uuid, TankType type);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, buuid uuid, TankType type);

	void SpawnTank(ObjRectangle rect, int color, int health, std::string name, std::string fraction, float speed,
	               buuid uuid, BonusEffectProperty effects, TankType type);
	std::unique_ptr<IInputProvider> GetInputProvider(TankType type);
	std::shared_ptr<BaseObj> CreateTank(TankType type, PawnProperty pawnProperty, BonusEffectProperty effects);

	void RespawnEnemyTanks(TankType type, buuid uuid);
	void RespawnPlayerTeam(TankType type, buuid uuid);
	void SetPlayerNeedRespawn();
	static std::string GetCurrentTimeString();

	void ResetRespawnStat();
	void RespawnTanks();
	void RespawnClient(TankType type, buuid uuid);
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

	[[nodiscard]] int GetEnemyRespawnResource() const { return _respawnResource[RespawnResource::ENEMY_ALL]; }
	[[nodiscard]] int GetPlayerOneRespawnResource() const { return _respawnResource[RespawnResource::PLAYER_ONE]; }
	[[nodiscard]] int GetPlayerTwoRespawnResource() const { return _respawnResource[RespawnResource::PLAYER_TWO]; }
	[[nodiscard]] int IsEnemyNeedRespawn() const { return _enemyNeedRespawn; }

	[[nodiscard]] bool IsEnemyOneNeedRespawn() const { return _slots[0].isAvailable; }
	//TODO: remove this after tests fixes
	[[nodiscard]] bool IsEnemyTwoNeedRespawn() const { return _slots[1].isAvailable; }
	//TODO: remove this after tests fixes
	[[nodiscard]] bool IsEnemyThreeNeedRespawn() const { return _slots[2].isAvailable; }
	//TODO: remove this after tests fixes
	[[nodiscard]] bool IsEnemyFourNeedRespawn() const { return _slots[3].isAvailable; }
	//TODO: remove this after tests fixes

	[[nodiscard]] bool IsPlayerOneNeedRespawn() const { return _slots[4].isAvailable; }
	[[nodiscard]] bool IsPlayerTwoNeedRespawn() const { return _slots[5].isAvailable; }

	//TODO: FOR UNIT TESTING ONLY
	void SetSlotNeedRespawn(int slotIndex);
};
