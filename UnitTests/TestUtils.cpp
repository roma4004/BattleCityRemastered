#include "TestUtils.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/input/InputProviderForBot.h"
#include "components/input/InputProviderForPlayer.h"
#include "entities/BaseObj.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/Tank.h"
#include "entities/pawns/TankResetProperty.h"
#include "enums/InputChannel.h"
#include "enums/TankType.h"
#include "utils/UuidUtils.h"
#include <chrono>

using namespace std::chrono_literals;

void TestUtils::ApplyGameMode(const std::shared_ptr<EventSystem>& events,
							  const std::vector<std::shared_ptr<BaseObj>>& allObjects, GameConfig& gameConfig,
							  const GameMode gameMode, std::shared_ptr<RespawnManager>& respawnManager,
							  std::shared_ptr<TankSpawner>& tankSpawner)
{
	gameConfig.gameMode = gameMode;
	//NOTE: the enemy throttle is wall-clock time, and a test has none to spare
	gameConfig.enemySpawnCooldown = 0ms;
	respawnManager = std::make_shared<RespawnManager>(events, gameMode);
	//NOTE: SpawnManager keeps the pools across mode changes; a fixture has none, so they live with the spawner
	const auto bulletPool{std::make_shared<BulletPool>(events, allObjects, gameConfig)};
	tankSpawner = std::make_shared<TankSpawner>(gameConfig, allObjects, events,
												std::make_shared<TankPool>(events, allObjects, gameConfig,
																		   bulletPool));
}

namespace
{
//NOTE: the seat says who drives, the type says what kind of tank sits in it - a bot in a player seat is a coop one
[[nodiscard]] TankType BotTypeOf(const Author author)
{
	switch (author)
	{
		case Author::Enemy1:
			return TankType::ENEMY1;
		case Author::Enemy2:
			return TankType::ENEMY2;
		case Author::Enemy3:
			return TankType::ENEMY3;
		case Author::Enemy4:
			return TankType::ENEMY4;
		case Author::Player2:
			return TankType::COOP2;
		default:
			return TankType::COOP1;
	}
}

[[nodiscard]] TankResetProperty MakeResetProperty(const ObjRectangle rect, const int health, const TankType type,
												  const Direction dir, const unsigned short tier, const TankModel model)
{
	return TankResetProperty{.uuid = UuidUtils::GetRandomUuid(),
							 .rect = rect,
							 .health = health,
							 .type = type,
							 .model = model,
							 .dir = dir,
							 .tier = tier};
}
}//namespace

std::shared_ptr<Tank> TestUtils::CreateBot(
		const ObjRectangle rect, const int health, const Author author,
		const std::vector<std::shared_ptr<BaseObj>>& allObjects, const std::shared_ptr<EventSystem>& events,
		const Direction dir, const std::shared_ptr<TankPool>& tankPool, const GameConfig& gameConfig,
		const unsigned short tier, const TankModel model)
{
	auto tank{tankPool->SpawnTank(MakeResetProperty(rect, health, BotTypeOf(author), dir, tier, model),
								  std::make_unique<InputProviderForBot>(allObjects, gameConfig))};
	events->EmitEvent(AddToSpawnQueueEvent{.obj = tank});

	return tank;
}

std::shared_ptr<Tank> TestUtils::CreatePlayer(
		const ObjRectangle rect, const int health, const Author author,
		const std::vector<std::shared_ptr<BaseObj>>&, const std::shared_ptr<EventSystem>& events,
		const Direction dir, const std::shared_ptr<TankPool>& tankPool, const GameConfig&,
		const unsigned short tier, const TankModel model)
{
	const TankType type{author == Author::Player2 ? TankType::PLAYER2 : TankType::PLAYER1};
	const InputChannel channel{author == Author::Player1 ? InputChannel::LocalP1 : InputChannel::LocalP2};

	auto tank{tankPool->SpawnTank(MakeResetProperty(rect, health, type, dir, tier, model),
								  std::make_unique<InputProviderForPlayer>(events, channel))};
	events->EmitEvent(AddToSpawnQueueEvent{.obj = tank});

	return tank;
}

std::shared_ptr<Bullet> TestUtils::CreateBullet(const ObjRectangle rect, const int health,
												const std::shared_ptr<BulletPool>& bulletPool,
												const std::shared_ptr<EventSystem>& events,
												const BulletCaliber& caliber, const Direction dir,
												const Author author, const Uuid& authorUuid)
{
	const BulletResetProperty property{.rect = rect,
									   .dir = dir,
									   .health = health,
									   .author = author,
									   .authorUuid = authorUuid,
									   .caliber = caliber};

	auto bullet{bulletPool->SpawnBullet(property)};
	events->EmitEvent(AddToSpawnQueueEvent{.obj = bullet});

	return bullet;
}
