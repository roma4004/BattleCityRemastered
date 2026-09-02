#include "TestUtils.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/input/InputProviderForPlayer.h"
#include "entities/BaseObj.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"

void TestUtils::ApplyGameMode(const std::shared_ptr<EventSystem>& events,
							  const std::vector<std::shared_ptr<BaseObj>>& allObjects, GameConfig& gameConfig,
							  const GameMode gameMode, std::shared_ptr<RespawnManager>& respawnManager,
							  std::shared_ptr<TankSpawner>& tankSpawner)
{
	gameConfig.gameMode = gameMode;
	//NOTE: the enemy throttle is wall-clock time, and a test has none to spare
	gameConfig.enemySpawnCooldown = std::chrono::milliseconds{0};
	respawnManager = std::make_shared<RespawnManager>(events, gameMode);
	tankSpawner = std::make_shared<TankSpawner>(gameConfig, allObjects, events);
}

template<>
[[nodiscard]] std::shared_ptr<Player> TestUtils::CreateTank<Player>(
		const ObjRectangle rect, const int tankHealth, const Uuid uuid, const Author author, const Faction faction,
		const std::vector<std::shared_ptr<BaseObj>>& allObjects, std::shared_ptr<EventSystem> events,
		const unsigned short tier, const double tankSpeed, const Direction dir, const GameMode gameMode,
		std::shared_ptr<BulletPool> bulletPool, const GameConfig& gameConfig)
{
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = tankHealth,
			.uuid = uuid,
			.name = std::string{ToString(author)},
			.faction = faction};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = allObjects,
			.events = events,
			.tier = tier,
			.speed = tankSpeed,
			.dir = dir,
			.gameMode = gameMode,
			.author = author};

	const InputChannel channel{author == Author::Player1 ? InputChannel::LocalP1 : InputChannel::LocalP2};
	std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayer>(events, channel);

	auto player = std::make_shared<Player>(std::move(pawnProperty), bulletPool, std::move(inputProvider),
										   gameConfig);
	player->Activate();

	return player;
}
