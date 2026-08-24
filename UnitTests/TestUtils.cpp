#include "TestUtils.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/input/InputProviderForPlayerOne.h"
#include "components/input/InputProviderForPlayerTwo.h"
#include "entities/BaseObj.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"

void TestUtils::ApplyGameMode(const std::shared_ptr<EventSystem>& events,
							   std::vector<std::shared_ptr<BaseObj>>* allObjects, GameConfig& gameConfig,
							   const GameMode gameMode, std::shared_ptr<RespawnManager>& respawnManager,
							   std::shared_ptr<TankSpawner>& tankSpawner)
{
	gameConfig.gameMode = gameMode;
	respawnManager = std::make_shared<RespawnManager>(events, gameMode);
	tankSpawner = std::make_shared<TankSpawner>(gameConfig, allObjects, events);
}

template<>
[[nodiscard]] std::shared_ptr<Player> TestUtils::CreateTank<Player>(
		ObjRectangle rect, int tankHealth, Uuid uuid, std::string name, std::string fraction,
		std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events, unsigned short tier,
		float tankSpeed, Direction dir, GameMode gameMode, std::shared_ptr<BulletPool> bulletPool,
		GameConfig& gameConfig)
{
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = tankHealth,
			.uuid = uuid,
			.name = name,
			.fraction = fraction};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = allObjects,
			.events = events,
			.tier = tier,
			.speed = tankSpeed,
			.dir = dir,
			.gameMode = gameMode};

	if (name == "Player1")
	{
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(events);

		return std::make_shared<Player>(std::move(pawnProperty), bulletPool, std::move(inputProvider), gameConfig);
	}

	std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerTwo>(events);

	return std::make_shared<Player>(std::move(pawnProperty), bulletPool, std::move(inputProvider), gameConfig);
}
