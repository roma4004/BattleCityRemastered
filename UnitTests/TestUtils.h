#pragma once
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/managers/TextureManager.h"
#include "entities/BaseObj.h"
#include "entities/pawns/CoopBot.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"

using buuid = boost::uuids::uuid;

class TestUtils
{
public:
	template<class T>
	[[nodiscard]] static std::shared_ptr<T> CreateTank(
			ObjRectangle rect, int tankHealth, buuid uuid, std::string name, std::string fraction,
			std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events, unsigned int tier,
			float tankSpeed, Direction dir, GameMode gameMode, std::shared_ptr<BulletPool> bulletPool,
			GameConfig& gameConfig);
};

template<class T>
std::shared_ptr<T> TestUtils::CreateTank(ObjRectangle rect, int tankHealth, buuid uuid, std::string name,
										 std::string fraction, std::vector<std::shared_ptr<BaseObj>>* allObjects,
										 std::shared_ptr<EventSystem> events,
										 unsigned int tier, float tankSpeed, Direction dir, GameMode gameMode,
										 std::shared_ptr<BulletPool> bulletPool,
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
	constexpr bool enableByDefault{true};

	return std::make_shared<T>(std::move(pawnProperty), bulletPool, gameConfig, enableByDefault);
}

template<>
[[nodiscard]] std::shared_ptr<Player> TestUtils::CreateTank<Player>(
		ObjRectangle rect, int tankHealth, buuid uuid, std::string name, std::string fraction,
		std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events, unsigned int tier,
		float tankSpeed, Direction dir, GameMode gameMode, std::shared_ptr<BulletPool> bulletPool,
		GameConfig& gameConfig);
