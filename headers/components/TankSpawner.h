#pragma once

#include <memory>
#include <random>
#include <boost/uuid/uuid.hpp>

#include "SDL.h"

enum TankType : char8_t;
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
	std::shared_ptr<SDL_Texture> _atlasTexture{nullptr};
	std::shared_ptr<SDL_Renderer> _renderer{nullptr};

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

	boost::uuids::uuid _enemyOneUuid;
	boost::uuids::uuid _enemyTwoUuid;
	boost::uuids::uuid _enemyThreeUuid;
	boost::uuids::uuid _enemyFourUuid;
	boost::uuids::uuid _playerOneUuid;
	boost::uuids::uuid _playerTwoUuid;

	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusTank(const std::string& author, const std::string& fraction);

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void SpawnEnemy(boost::uuids::uuid uuid, float speed, int health, TankType type);
	void SetEnemyNeedRespawn();

	void SpawnPlayer(ObjRectangle rect, float speed, int health, boost::uuids::uuid uuid, TankType type);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, boost::uuids::uuid uuid, TankType type);
	void RespawnEnemyTanks(TankType type, boost::uuids::uuid uuid);
	void RespawnPlayerTeam(TankType type, boost::uuids::uuid uuid);
	void SetPlayerNeedRespawn();
	static std::string GetCurrentTimeString();

	void ResetRespawnStat();
	void RespawnTanks();
	void RespawnClient(TankType type, boost::uuids::uuid uuid);
	void ResetSpawn();

	void IncreaseEnemyRespawnResource();
	void IncreasePlayerOneRespawnResource();
	void IncreasePlayerTwoRespawnResource();

	void DecreaseEnemyRespawnResource();
	void DecreasePlayerOneRespawnResource();
	void DecreasePlayerTwoRespawnResource();
	void OnTankSpawn(const std::string& whoSpawn);
	void OnEnemyTankDied(const std::string& whoDied);
	void OnPlayerTankDied(const std::string& whoDied);
	void OnTankDied(const std::string& whoDied);

public:
	TankSpawner(std::shared_ptr<Window> window, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool,
				std::shared_ptr<SDL_Texture> textureCollection,
				std::shared_ptr<SDL_Renderer> _renderer);

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
