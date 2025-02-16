#pragma once

#include "Point.h"

#include <memory>
#include <random>

class BaseObj;
class BulletPool;
class EventSystem;

enum GameMode : char8_t;

class TankSpawner final
{
	UPoint _windowSize{.x = 0, .y = 0};
	std::string _name = "TankSpawner";
	std::shared_ptr<int[]> _windowBuffer{nullptr};
	GameMode _currentMode;

	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

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
	bool _coopOneAINeedRespawn{false};
	bool _coopTwoAINeedRespawn{false};

	void Subscribe();
	void Unsubscribe() const;

	void SpawnEnemy(int index, float gridOffset, float speed, int health, float size, bool isNetworkControlled);
	void SpawnEnemyTanks(float gridOffset, float speed, int health, float size);

	void SpawnPlayer1(float gridOffset, float speed, int health, float size, bool isNetworkControlled);
	void SpawnPlayer2(float gridOffset, float speed, int health, float size, bool isNetworkControlled);
	void SpawnCoop1(float gridOffset, float speed, int health, float size);
	void SpawnCoop2(float gridOffset, float speed, int health, float size);
	void SpawnPlayerTanks(float gridOffset, float speed, int health, float size);

	void ResetRespawnStat();
	void RespawnTanks();
	void InitialSpawn();

	void IncreaseEnemyRespawnResource();
	void IncreasePlayerOneRespawnResource();
	void IncreasePlayerTwoRespawnResource();

	void DecreaseEnemyRespawnResource();
	void DecreasePlayerOneRespawnResource();
	void DecreasePlayerTwoRespawnResource();

public:
	TankSpawner(UPoint windowSize, std::shared_ptr<int[]> windowBuffer,
	            std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	            std::shared_ptr<BulletPool> bulletPool);

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
	[[nodiscard]] bool IsCoopOneAINeedRespawn() const { return _coopOneAINeedRespawn; }
	[[nodiscard]] bool IsCoopTwoAINeedRespawn() const { return _coopTwoAINeedRespawn; }
};
