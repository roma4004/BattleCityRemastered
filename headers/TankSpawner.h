#pragma once

#include "BaseObj.h"
#include "BulletPool.h"
#include "EventSystem.h"
#include "Point.h"
#include "bonuses/BonusSystem.h"

#include <random>

class TankSpawner final
{
	UPoint _windowSize{0, 0};
	std::string _name = "TankSpawner";
	int* _windowBuffer{nullptr};
	GameMode _currentMode{Demo};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects;

	std::shared_ptr<EventSystem> _events;

	std::shared_ptr<BulletPool> _bulletPool;

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

	void SpawnEnemy(int index, float gridOffset, float speed, int health, float size);
	void SpawnEnemyTanks(float gridOffset, float speed, int health, float size);

	void SpawnPlayer1(float gridOffset, float speed, int health, float size);
	void SpawnPlayer2(float gridOffset, float speed, int health, float size);
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
	TankSpawner(UPoint windowSize, int* windowBuffer, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            const std::shared_ptr<EventSystem>& events);

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
