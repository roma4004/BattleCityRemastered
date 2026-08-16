#pragma once
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"

using buuid = boost::uuids::uuid;

class TestUtils
{
public:
	[[nodiscard]] static EventSubscription WireSpawnQueue(const std::shared_ptr<EventSystem>& events,
														   std::vector<std::shared_ptr<BaseObj>>* allObjects)
	{
		return events->AddListener([allObjects](const AddToSpawnQueueEvent& event)
		{
			allObjects->emplace_back(event.obj);
		});
	}

	template<class T>
	[[nodiscard]] static std::shared_ptr<T> CreateTank(
			ObjRectangle rect, int health, buuid uuid, std::string name, std::string fraction,
			std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events, unsigned short tier,
			float tankSpeed, Direction dir, GameMode gameMode, std::shared_ptr<BulletPool> bulletPool,
			GameConfig& gameConfig);

	[[nodiscard]] static std::shared_ptr<Bullet> CreateBullet(
			ObjRectangle rect, int health, buuid uuid, std::string name, std::string fraction,
			std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
			const BulletCalibre& calibre, Direction dir, GameMode gameMode, GameConfig& gameConfig, std::string author)
	{
		BaseObjProperty baseObjProperty{
				.rect = rect,
				.health = health,
				.uuid = uuid,
				.name = std::move(name),
				.fraction = std::move(fraction)};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = allObjects,
				.events = events,
				.tier = calibre.tier,
				.speed = calibre.speed,
				.dir = dir,
				.gameMode = gameMode};

		constexpr bool enableByDefault{true};

		return std::make_shared<Bullet>(std::move(pawnProperty), gameConfig, calibre, std::move(author),
										enableByDefault);
	}
};

template<class T>
std::shared_ptr<T> TestUtils::CreateTank(ObjRectangle rect, int health, buuid uuid, std::string name,
										 std::string fraction, std::vector<std::shared_ptr<BaseObj>>* allObjects,
										 std::shared_ptr<EventSystem> events,
										 unsigned short tier, float tankSpeed, Direction dir, GameMode gameMode,
										 std::shared_ptr<BulletPool> bulletPool,
										 GameConfig& gameConfig)
{
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = health,
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

	return std::make_shared<T>(std::move(pawnProperty), bulletPool, gameConfig);
}

template<>
[[nodiscard]] std::shared_ptr<Player> TestUtils::CreateTank<Player>(
		ObjRectangle rect, int tankHealth, buuid uuid, std::string name, std::string fraction,
		std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events, unsigned short tier,
		float tankSpeed, Direction dir, GameMode gameMode, std::shared_ptr<BulletPool> bulletPool,
		GameConfig& gameConfig);
