#pragma once

#include "GameMode.h"
#include "application/Window.h"

#include <memory>
#include <random>

class BaseObj;
class BulletPool;
class EventSystem;

enum GameMode : char8_t;

class TankSpawner final
{
	std::string _name{"TankSpawner"};
	GameMode _currentMode{Demo};

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

	void OnBonusTank(const std::string& author, const std::string& fraction);

	void Subscribe();
	void SubscribeAsClient();
	void SubscribeBonus();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;
	void UnsubscribeBonus() const;

	void SpawnEnemy(int index, float gridOffset, float speed, int health, float size);
	void SetEnemyNeedRespawn();

	void SpawnPlayer1(float gridOffset, float speed, int health, float size);
	void SpawnPlayer2(float gridOffset, float speed, int health, float size);
	void SpawnCoopBot1(float gridOffset, float speed, int health, float size);
	void SpawnCoopBot2(float gridOffset, float speed, int health, float size);
	void RespawnEnemyTanks(int index);
	void RespawnPlayerTanks(int index);
	void RespawnCoopTanks(int index);
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
