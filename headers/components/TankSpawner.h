#pragma once

#include <memory>
#include <random>

struct ObjRectangle;
enum GameMode : char8_t;
struct Window;
class BaseObj;
class BulletPool;
class EventSystem;

class TankSpawner final
{
	std::string _name{"TankSpawner"};
	GameMode _gameMode{};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};

	std::random_device _rd;

	// TODO: use std::atomic when multithreading is used
	int _enemyRespawnResource{20};
	int _playerOneRespawnResource{3};
	int _playerTwoRespawnResource{3};

	bool _enemyOneNeedRespawn{false};
	bool _enemyTwoNeedRespawn{false};
	bool _enemyThreeNeedRespawn{false};
	bool _enemyFourNeedRespawn{false};
	bool _playerOneNeedRespawn{false};
	bool _playerTwoNeedRespawn{false};
	bool _coopBotOneNeedRespawn{false};
	bool _coopBotTwoNeedRespawn{false};

	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusTank(const std::string& author, const std::string& fraction);

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void SpawnEnemy(int id, float speed, int health);
	void SetEnemyNeedRespawn();

	void SpawnPlayer(ObjRectangle rect, float speed, int health, int id);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, int id);
	void RespawnEnemyTanks(int id);
	void RespawnPlayerTanks(int id);
	void RespawnCoopTanks(int id);
	void SetPlayerNeedRespawn();

	void ResetRespawnStat();
	void RespawnTanks();
	void ResetSpawn();

	void IncreaseEnemyRespawnResource();
	void IncreasePlayerOneRespawnResource();
	void IncreasePlayerTwoRespawnResource();

	void DecreaseEnemyRespawnResource();
	void DecreasePlayerOneRespawnResource();
	void DecreasePlayerTwoRespawnResource();
	void OnTankSpawn(const std::string& whoSpawn);
	void OnTankDied(const std::string& whoDied);

public:
	TankSpawner(std::shared_ptr<Window> window, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool);

	~TankSpawner();

	[[nodiscard]] int GetEnemyRespawnResource() const { return _enemyRespawnResource; }
	[[nodiscard]] int GetPlayerOneRespawnResource() const { return _playerOneRespawnResource; }
	[[nodiscard]] int GetPlayerTwoRespawnResource() const { return _playerTwoRespawnResource; }

	[[nodiscard]] bool IsEnemyOneNeedRespawn() const { return _enemyOneNeedRespawn; }
	[[nodiscard]] bool IsEnemyTwoNeedRespawn() const { return _enemyTwoNeedRespawn; }
	[[nodiscard]] bool IsEnemyThreeNeedRespawn() const { return _enemyThreeNeedRespawn; }
	[[nodiscard]] bool IsEnemyFourNeedRespawn() const { return _enemyFourNeedRespawn; }

	[[nodiscard]] bool IsPlayerOneNeedRespawn() const { return _playerOneNeedRespawn; }
	[[nodiscard]] bool IsPlayerTwoNeedRespawn() const { return _playerTwoNeedRespawn; }
	[[nodiscard]] bool IsCoopBotOneNeedRespawn() const { return _coopBotOneNeedRespawn; }
	[[nodiscard]] bool IsCoopBotTwoNeedRespawn() const { return _coopBotTwoNeedRespawn; }
};
