#pragma once

#include "Point.h"
#include "managers/RespawnManager.h"
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

	GameMode _gameMode{};

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void SpawnEnemy(buuid uuid, TankType type, float speed, int health, bool skipDelay = false);
	void SpawnPlayer(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);
	void SpawnCoopBot(ObjRectangle rect, float speed, int health, buuid uuid, TankType type, bool skipDelay = false);

	void SpawnTank(ObjRectangle rect, int color, int health, const std::string& name, std::string fraction, float speed,
	               buuid uuid, BonusEffectProperty effects, TankType type, bool skipDelay = false);
	[[nodiscard]] std::unique_ptr<IInputProvider> GetInputProvider(TankType type);
	[[nodiscard]] std::shared_ptr<Tank> CreateTank(TankType type, PawnProperty pawnProperty,
	                                               BonusEffectProperty effects);

	void RespawnEnemyTanks(TankType type, buuid uuid, bool skipDelay = false);
	void RespawnPlayerTeam(TankType type, buuid uuid, bool skipDelay = false);
	[[nodiscard]] static std::string GetCurrentTimeString();

	void OnClientRespawn(TankType type, buuid uuid, bool skipDelay = false);

public:
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};

	TankSpawner(UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            const std::shared_ptr<EventSystem>& events,
	            const std::shared_ptr<BonusEffectManager>& bonusEffectManager,
	            const std::shared_ptr<RespawnManager>& respawnManager);

	~TankSpawner();

	void RespawnTanks(bool skipDelay = false);
};
