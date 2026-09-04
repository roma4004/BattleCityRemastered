#pragma once
#include "geometry/Point.h"
#include <algorithm>
#include <cmath>
#include <ostream>
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "entities/obstacles/FortressWalls.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Tank.h"

class RespawnManager;
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

	//NOTE: the other half of what SpawnManager does on PostTickUpdate - a fixture that cares about
	//an object leaving the world has to sweep it out, otherwise nothing ever calls Deactivate
	[[nodiscard]] static EventSubscription WireWorldDisposal(const std::shared_ptr<EventSystem>& events,
															 std::vector<std::shared_ptr<BaseObj>>& allObjects)
	{
		return events->AddListener([objects = &allObjects](const PostTickUpdateEvent&)
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
		});
	}

	//NOTE: stands in for AnimationManager - a spawn burst takes a couple of seconds of frames, and a
	//test has none to spare, so it is over where it starts
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

	//NOTE: one tank either way - the two helpers differ only in who takes the wheel
	[[nodiscard]] static std::shared_ptr<Tank> CreateBot(
			ObjRectangle rect, int health, Uuid uuid, Author author, Faction faction,
			const std::vector<std::shared_ptr<BaseObj>>& allObjects, std::shared_ptr<EventSystem> events,
			unsigned short tier, double tankSpeed, Direction dir, std::shared_ptr<BulletPool> bulletPool,
			const GameConfig& gameConfig);

	[[nodiscard]] static std::shared_ptr<Tank> CreatePlayer(
			ObjRectangle rect, int health, Uuid uuid, Author author, Faction faction,
			const std::vector<std::shared_ptr<BaseObj>>& allObjects, std::shared_ptr<EventSystem> events,
			unsigned short tier, double tankSpeed, Direction dir, std::shared_ptr<BulletPool> bulletPool,
			const GameConfig& gameConfig);

	[[nodiscard]] static std::shared_ptr<Bullet> CreateBullet(
			ObjRectangle rect, const int health, const Uuid uuid, const Faction faction,
			const std::vector<std::shared_ptr<BaseObj>>& allObjects, std::shared_ptr<EventSystem> events,
			const BulletCalibre& calibre, const Direction dir, const GameConfig& gameConfig,
			const Author author)
	{
		BaseObjProperty baseObjProperty{
				.rect = rect,
				.health = health,
				.uuid = uuid,
				.faction = faction};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = allObjects,
				.events = events,
				.tier = calibre.tier,
				.speed = calibre.speed,
				.dir = dir,
				.author = author};

		auto bullet = std::make_shared<Bullet>(std::move(pawnProperty), gameConfig, calibre);
		bullet->Activate();

		return bullet;
	}
};

//NOTE: an epsilon comparison is not transitive, so it is no equivalence relation and has no
//business being spelled == on the type. Production never compares FPoints; the tests do.
[[nodiscard]] inline bool operator==(const FPoint& lhs, const FPoint& rhs) noexcept
{
	static constexpr double epsilon = 1e-4;
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
