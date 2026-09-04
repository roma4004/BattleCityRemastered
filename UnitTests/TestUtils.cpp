#include "TestUtils.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/input/InputProviderForBot.h"
#include "components/input/InputProviderForPlayer.h"
#include "entities/BaseObj.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Tank.h"

void TestUtils::ApplyGameMode(const std::shared_ptr<EventSystem>& events,
							  const std::vector<std::shared_ptr<BaseObj>>& allObjects, GameConfig& gameConfig,
							  const GameMode gameMode, std::shared_ptr<RespawnManager>& respawnManager,
							  std::shared_ptr<TankSpawner>& tankSpawner)
{
	gameConfig.gameMode = gameMode;
	//NOTE: the enemy throttle is wall-clock time, and a test has none to spare
	gameConfig.enemySpawnCooldown = std::chrono::milliseconds{0};
	respawnManager = std::make_shared<RespawnManager>(events, gameMode);
	//NOTE: production keeps the pools in SpawnManager so they survive a mode change; a fixture has
	//no such switch, so they live as long as the spawner
	const auto bulletPool{std::make_shared<BulletPool>(events, allObjects, gameConfig)};
	tankSpawner = std::make_shared<TankSpawner>(gameConfig, allObjects, events,
												std::make_shared<TankPool>(events, allObjects, gameConfig,
																		   bulletPool));
}

namespace
{
[[nodiscard]] PawnProperty MakePawnProperty(const ObjRectangle rect, const int health, const Uuid uuid,
										   const Author author, const Faction faction,
										   const std::vector<std::shared_ptr<BaseObj>>& allObjects,
										   const std::shared_ptr<EventSystem>& events, const unsigned short tier,
										   const double tankSpeed, const Direction dir)
{
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = health,
			.uuid = uuid,
			.faction = faction};

	return PawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = allObjects,
			.events = events,
			.tier = tier,
			.speed = tankSpeed,
			.dir = dir,
			.author = author};
}
}//namespace

std::shared_ptr<Tank> TestUtils::CreateBot(
		const ObjRectangle rect, const int health, const Uuid uuid, const Author author, const Faction faction,
		const std::vector<std::shared_ptr<BaseObj>>& allObjects, std::shared_ptr<EventSystem> events,
		const unsigned short tier, const double tankSpeed, const Direction dir, std::shared_ptr<BulletPool> bulletPool,
		const GameConfig& gameConfig)
{
	PawnProperty pawnProperty{MakePawnProperty(rect, health, uuid, author, faction, allObjects, events, tier,
											   tankSpeed, dir)};

	auto tank = std::make_shared<Tank>(std::move(pawnProperty), bulletPool,
									   std::make_unique<InputProviderForBot>(allObjects, gameConfig), gameConfig);
	tank->Activate();

	return tank;
}

std::shared_ptr<Tank> TestUtils::CreatePlayer(
		const ObjRectangle rect, const int health, const Uuid uuid, const Author author, const Faction faction,
		const std::vector<std::shared_ptr<BaseObj>>& allObjects, std::shared_ptr<EventSystem> events,
		const unsigned short tier, const double tankSpeed, const Direction dir, std::shared_ptr<BulletPool> bulletPool,
		const GameConfig& gameConfig)
{
	PawnProperty pawnProperty{MakePawnProperty(rect, health, uuid, author, faction, allObjects, events, tier,
											   tankSpeed, dir)};

	const InputChannel channel{author == Author::Player1 ? InputChannel::LocalP1 : InputChannel::LocalP2};

	auto tank = std::make_shared<Tank>(std::move(pawnProperty), bulletPool,
									   std::make_unique<InputProviderForPlayer>(events, channel), gameConfig);
	tank->Activate();

	return tank;
}
