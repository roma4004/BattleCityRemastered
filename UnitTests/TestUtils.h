#pragma once
#include "geometry/Point.h"
#include <algorithm>
#include <cmath>
#include <ostream>
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "entities/obstacles/FortressWalls.h"
#include "components/events/SpawnEvents.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"

class RespawnManager;
class TankSpawner;

class TestUtils
{
public:
	static void ApplyGameMode(const std::shared_ptr<EventSystem>& events,
							   std::vector<std::shared_ptr<BaseObj>>* allObjects, GameConfig& gameConfig,
							   GameMode gameMode, std::shared_ptr<RespawnManager>& respawnManager,
							   std::shared_ptr<TankSpawner>& tankSpawner);

	[[nodiscard]] static EventSubscription WireSpawnQueue(const std::shared_ptr<EventSystem>& events,
														  std::vector<std::shared_ptr<BaseObj>>* allObjects)
	{
		return events->AddListener([allObjects](const AddToSpawnQueueEvent& event)
		{
			allObjects->emplace_back(event.obj);
		});
	}

	[[nodiscard]] static EventSubscription TrackFortressWall(const std::shared_ptr<EventSystem>& events,
															 std::shared_ptr<BaseObj>* out)
	{
		return events->AddListener([out](const AddToSpawnQueueEvent& event)
		{
			if (dynamic_cast<IFortress*>(event.obj.get()) != nullptr)
			{
				*out = event.obj;
			}
		});
	}

	template<class T>
	[[nodiscard]] static std::shared_ptr<T> CreateTank(
			ObjRectangle rect, int health, Uuid uuid, std::string name, std::string fraction,
			std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events, unsigned short tier,
			float tankSpeed, Direction dir, GameMode gameMode, std::shared_ptr<BulletPool> bulletPool,
			GameConfig& gameConfig);

	[[nodiscard]] static std::shared_ptr<Bullet> CreateBullet(
			ObjRectangle rect, int health, Uuid uuid, std::string name, std::string fraction,
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
std::shared_ptr<T> TestUtils::CreateTank(ObjRectangle rect, int health, Uuid uuid, std::string name,
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
		ObjRectangle rect, int tankHealth, Uuid uuid, std::string name, std::string fraction,
		std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events, unsigned short tier,
		float tankSpeed, Direction dir, GameMode gameMode, std::shared_ptr<BulletPool> bulletPool,
		GameConfig& gameConfig);

//NOTE: an epsilon comparison is not transitive, so it is no equivalence relation and has no
//business being spelled == on the type. Production never compares FPoints; the tests do.
[[nodiscard]] inline bool operator==(const FPoint& lhs, const FPoint& rhs) noexcept
{
	static constexpr float epsilon = 1e-4f;
	return std::abs(lhs.x - rhs.x) < epsilon && std::abs(lhs.y - rhs.y) < epsilon;
}

//NOTE: here, not in Point.h, to keep <ostream> out of the ~40 files that never print a point.
//Found by ADL, so the test TU has to include this header.
inline void PrintTo(const FPoint& point, std::ostream* os)
{
	*os << "FPoint(x: " << point.x << ", y: " << point.y << ")";
}

inline void PrintTo(const Point& point, std::ostream* os)
{
	*os << "Point(x: " << point.x << ", y: " << point.y << ")";
}

inline void PrintTo(const UPoint& point, std::ostream* os)
{
	*os << "UPoint(x: " << point.x << ", y: " << point.y << ")";
}
