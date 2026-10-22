#pragma once
#include "utils/MathUtils.h"
#include "geometry/Point.h"
#include <algorithm>
#include <cmath>
#include <ostream>
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "entities/obstacles/FortressWalls.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObj.h"
#include "enums/ObstacleType.h"
#include "enums/TankModel.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Tank.h"
#include "utils/UuidUtils.h"

class BulletPool;
class RespawnManager;
class TankPool;
class TankSpawner;

class TestUtils
{
public:
	static void ApplyGameMode(const std::shared_ptr<EventSystem>& events,
							  const std::vector<std::shared_ptr<BaseObj>>& allObjects, GameConfig& gameConfig,
							  GameMode gameMode, std::shared_ptr<RespawnManager>& respawnManager,
							  std::shared_ptr<TankSpawner>& tankSpawner);

	[[nodiscard]] static EventSubscription WireSpawnQueue(const std::shared_ptr<EventSystem>& events,
														  std::vector<std::shared_ptr<BaseObj>>& allObjects)
	{
		return events->AddListener([objects = &allObjects](const AddToSpawnQueueEvent& event)
		{
			event.obj->Activate();
			objects->emplace_back(event.obj);
		});
	}

	//NOTE: stands in for SpawnManager's sweep on PostTickUpdate - without it nothing calls Deactivate
	[[nodiscard]] static EventSubscription WireWorldDisposal(const std::shared_ptr<EventSystem>& events,
															 std::vector<std::shared_ptr<BaseObj>>& allObjects)
	{
		return events->AddListener([objects = &allObjects, events](const PostTickUpdateEvent&)
		{
			const auto isDead = [](const std::shared_ptr<BaseObj>& obj)
			{
				return obj == nullptr || obj->GetIsAlive() == false;
			};

			for (const std::shared_ptr<BaseObj>& obj: *objects)
			{
				if (obj != nullptr && obj->GetIsAlive() == false)
				{
					obj->Deactivate();
				}
			}

			std::erase_if(*objects, isDead);

			events->EmitEvent(DeadObjectsSweptEvent{});
		});
	}

	//NOTE: stands in for AnimationManager - a spawn burst ends the moment it starts
	[[nodiscard]] static std::vector<EventSubscription> WireInstantSpawnAnimations(
			const std::shared_ptr<EventSystem>& events)
	{
		std::vector<EventSubscription> subs{};
		subs.push_back(events->AddListener([events](const AnimationCreateTankSpawnEvent& event)
		{
			events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = event.uuid});
		}));
		subs.push_back(events->AddListener([events](const AnimationCreateBonusSpawnEvent& event)
		{
			events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = event.uuid});
		}));

		return subs;
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

	//NOTE: laid cell by cell, the way a map does it - one stretched obstacle is a shape the game never makes
	static void SpawnObstacleArea(const std::shared_ptr<EventSystem>& events,
								  const std::vector<std::shared_ptr<BaseObj>>& allObjects, const ObjRectangle area,
								  const ObstacleType type, const GameConfig& gameConfig)
	{
		const double side{gameConfig.gridOffset * ObstacleCellSpan(type)};
		for (double y{area.y}; y < area.Bottom(); y += side)
		{
			for (double x{area.x}; x < area.Right(); x += side)
			{
				std::ignore = SpawnObstacle(events, allObjects, ObjRectangle{.x = x, .y = y, .w = side, .h = side},
											type);
			}
		}
	}

	//NOTE: the game never builds an obstacle itself, ObstacleSpawner does; the rect overload is for geometry cases
	[[nodiscard]] static std::shared_ptr<BaseObj> SpawnObstacle(const std::shared_ptr<EventSystem>& events,
																const std::vector<std::shared_ptr<BaseObj>>& allObjects,
																const ObjRectangle rect, const ObstacleType type)
	{
		events->EmitEvent(SpawnObstacleEvent{.rect = rect, .type = type});

		return allObjects.empty() ? nullptr : allObjects.back();
	}

	[[nodiscard]] static std::shared_ptr<BaseObj> SpawnObstacle(const std::shared_ptr<EventSystem>& events,
																const std::vector<std::shared_ptr<BaseObj>>& allObjects,
																const FPoint pos, const ObstacleType type,
																const GameConfig& gameConfig)
	{
		const double side{gameConfig.gridOffset * ObstacleCellSpan(type)};

		return SpawnObstacle(events, allObjects, ObjRectangle{.x = pos.x, .y = pos.y, .w = side, .h = side}, type);
	}

	//NOTE: tanks come from the pools through the spawn queue, as in the game; the helpers differ only in the driver
	[[nodiscard]] static std::shared_ptr<Tank> CreateBot(
			ObjRectangle rect, unsigned int health, Author author,
			const std::vector<std::shared_ptr<BaseObj>>& allObjects, const std::shared_ptr<EventSystem>& events,
			Direction dir, const std::shared_ptr<TankPool>& tankPool, const GameConfig& gameConfig,
			unsigned short tier = 1u, TankModel model = TankModel::Basic);

	[[nodiscard]] static std::shared_ptr<Tank> CreatePlayer(
			ObjRectangle rect, unsigned int health, Author author,
			const std::vector<std::shared_ptr<BaseObj>>& allObjects, const std::shared_ptr<EventSystem>& events,
			Direction dir, const std::shared_ptr<TankPool>& tankPool, const GameConfig& gameConfig,
			unsigned short tier = 1u, TankModel model = TankModel::Player);

	[[nodiscard]] static std::shared_ptr<Bullet> CreateBullet(
			ObjRectangle rect, const std::shared_ptr<BulletPool>& bulletPool,
			const std::shared_ptr<EventSystem>& events, const BulletCaliber& caliber, Direction dir,
			Author author, const Uuid& authorUuid = {});
};

//NOTE: test-only - an epsilon comparison is not transitive, so it has no business as == on the type itself
[[nodiscard]] inline bool operator==(const FPoint& lhs, const FPoint& rhs) noexcept
{
	static constexpr double epsilon{1e-4};
	return MathUtils::AreEqualAbsolute(lhs.x, rhs.x, epsilon) && MathUtils::AreEqualAbsolute(lhs.y, rhs.y, epsilon);
}

//NOTE: gtest finds these by ADL, so a test printing a point includes this header
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
