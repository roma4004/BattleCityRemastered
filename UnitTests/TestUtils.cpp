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
[[nodiscard]] PawnProperty MakePawnProperty(const ObjRectangle rect, const int health, const Author author,
											const std::vector<std::shared_ptr<BaseObj>>& allObjects,
											const std::shared_ptr<EventSystem>& events, const unsigned short tier,
											const double tankSpeed, const Direction dir)
{
	const BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = health,
			.uuid = UuidUtils::GetRandomUuid(),
			.faction = FactionOf(author)};

	return PawnProperty{
			.baseObjProperty = baseObjProperty,
			.allObjects = allObjects,
			.events = events,
			.tier = tier,
			.speed = tankSpeed,
			.dir = dir,
			.author = author};
}
}//namespace

std::shared_ptr<Tank> TestUtils::CreateBot(
		const ObjRectangle rect, const int health, const Author author,
		const std::vector<std::shared_ptr<BaseObj>>& allObjects, const std::shared_ptr<EventSystem>& events,
		const Direction dir, const std::shared_ptr<BulletPool>& bulletPool, const GameConfig& gameConfig,
		const unsigned short tier)
{
	PawnProperty pawnProperty{MakePawnProperty(rect, health, author, allObjects, events, tier,
											   gameConfig.tankSpeed, dir)};

	auto tank{std::make_shared<Tank>(std::move(pawnProperty), bulletPool,
									 std::make_unique<InputProviderForBot>(allObjects, gameConfig), gameConfig)};
	tank->Activate();

	return tank;
}

std::shared_ptr<Tank> TestUtils::CreatePlayer(
		const ObjRectangle rect, const int health, const Author author,
		const std::vector<std::shared_ptr<BaseObj>>& allObjects, const std::shared_ptr<EventSystem>& events,
		const Direction dir, const std::shared_ptr<BulletPool>& bulletPool, const GameConfig& gameConfig,
		const unsigned short tier)
{
	PawnProperty pawnProperty{MakePawnProperty(rect, health, author, allObjects, events, tier,
											   gameConfig.tankSpeed, dir)};

	const InputChannel channel{author == Author::Player1 ? InputChannel::LocalP1 : InputChannel::LocalP2};

	auto tank{std::make_shared<Tank>(std::move(pawnProperty), bulletPool,
									 std::make_unique<InputProviderForPlayer>(events, channel), gameConfig)};
	tank->Activate();

	return tank;
}
