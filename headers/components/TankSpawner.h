#pragma once

#include <memory>
#include <random>
#include <boost/uuid/uuid.hpp>

#include "SDL.h"
#include "../BaseObjProperty.h"
#include "../ObjRectangle.h"
#include "../enums/Direction.h"
#include "../enums/GameMode.h"
#include "../enums/RespawnResource.h"
#include "../input/InputProviderForPlayerOne.h"
#include "../input/InputProviderForPlayerOneNet.h"
#include "../input/InputProviderForPlayerTwo.h"
#include "../input/InputProviderForPlayerTwoNet.h"
#include "../pawns/PawnProperty.h"
#include "../pawns/Player.h"

enum TankType : char8_t;
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
	std::vector<int> _respawnResource{20, 3, 3};
	int _enemyNeedRespawn{4};

	struct SpawnSlot
	{
		boost::uuids::uuid id{};
		bool isAvailable{false};
	};

	std::vector<SpawnSlot> _slots;

	void OnBonusGrenade(const std::string& author, const std::string& fraction);
	void OnBonusTank(const std::string& author, const std::string& fraction);

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void SpawnEnemy(boost::uuids::uuid uuid, float speed, int health);
	void SetEnemyNeedRespawn();

	void SpawnPlayer(ObjRectangle rect, float speed, int health, boost::uuids::uuid uuid, TankType type);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, boost::uuids::uuid uuid, TankType type);

	template<typename TTankType>
	void RespawnTank(ObjRectangle rect, int color, int health, std::string name, std::string fraction, float speed,
	                 boost::uuids::uuid uuid);

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
	void OnTankSpawn(const boost::uuids::uuid& uuid);
	void OnTankDied(const boost::uuids::uuid& uuid);

public:
	TankSpawner(std::shared_ptr<Window> window, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool,
	            std::shared_ptr<SDL_Texture> textureCollection,
	            std::shared_ptr<SDL_Renderer> renderer);

	~TankSpawner();

	[[nodiscard]] int GetEnemyRespawnResource() const { return _respawnResource[RespawnResource::ENEMY_ALL]; }
	[[nodiscard]] int GetPlayerOneRespawnResource() const { return _respawnResource[RespawnResource::PLAYER_ONE]; }
	[[nodiscard]] int GetPlayerTwoRespawnResource() const { return _respawnResource[RespawnResource::PLAYER_TWO]; }
	[[nodiscard]] int IsEnemyNeedRespawn() const { return _enemyNeedRespawn; }

	[[nodiscard]] bool IsEnemyOneNeedRespawn() const { return _respawnResource[RespawnResource::ENEMY_ALL]; }
	//TODO: remove this after tests fixes
	[[nodiscard]] bool IsEnemyTwoNeedRespawn() const { return _respawnResource[RespawnResource::ENEMY_ALL]; }
	//TODO: remove this after tests fixes
	[[nodiscard]] bool IsEnemyThreeNeedRespawn() const { return _respawnResource[RespawnResource::ENEMY_ALL]; }
	//TODO: remove this after tests fixes
	[[nodiscard]] bool IsEnemyFourNeedRespawn() const { return _respawnResource[RespawnResource::ENEMY_ALL]; }
	//TODO: remove this after tests fixes

	[[nodiscard]] bool IsPlayerOneNeedRespawn() const { return _respawnResource[RespawnResource::PLAYER_ONE]; }
	[[nodiscard]] bool IsPlayerTwoNeedRespawn() const { return _respawnResource[RespawnResource::PLAYER_TWO]; }
	[[nodiscard]] bool IsCoopBotOneNeedRespawn() const { return _respawnResource[RespawnResource::PLAYER_ONE]; }
	[[nodiscard]] bool IsCoopBotTwoNeedRespawn() const { return _respawnResource[RespawnResource::PLAYER_TWO]; }
};

// Include the template implementation
#include "TankSpawner.tpp"
