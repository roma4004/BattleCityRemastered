#include "network/ReplicationPublisher.h"
#include "enums/InputSignal.h"
#include "enums/PlayerTag.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/StatisticsEvents.h"
#include <mutex>
#include <utility>

namespace network::commands
{
ReplicationPublisher::ReplicationPublisher(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

CommandBatch ReplicationPublisher::TakeBatch()
{
	CommandBatch batch;
	std::scoped_lock lock(_batchWriteMutex);
	std::swap(batch, _batch);

	return batch;
}

void ReplicationPublisher::Subscribe()
{
	Bind<PauseStatusEvent>([](const auto& e)
	{
		return KeyStateChange{.tag = PlayerTag::None,
							  .action = InputSignal::PauseStatus,
							  .isPressed = e.isPaused};
	});
	Bind<GameFinishedEvent>([](const auto& e) { return GameStateChange{.state = e.state}; });

	Bind<PosChangedEvent>([](const auto& e)
	{
		return PositionChange{.who = e.who, .pos = e.pos, .dir = e.dir, .uuid = e.uuid};
	});
	Bind<TankShotEvent>([](const auto& e) { return TankShot{.who = e.who, .dir = e.dir, .uuid = e.bulletUuid}; });
	Bind<HealthChangedEvent>([](const auto& e)
	{
		return HealthChange{.who = e.who, .health = e.health, .uuid = e.uuid};
	});
	Bind<DespawnedEvent>([](const auto& e)
	{
		return Despawn{.who = e.who, .uuid = e.uuid, .reason = e.reason};
	});
	Bind<TankRespawnedEvent>([](const auto& e)
	{
		return RespawnTank{.tankType = e.type, .uuid = e.uuid, .pos = e.pos};
	});
	Bind<ObstacleSpawnedEvent>([](const auto& e)
	{
		return ObstacleSpawn{.pos = e.pos, .obstacleType = e.type, .uuid = e.uuid};
	});
	Bind<TankSpawnCompletedEvent>([](const auto& e) { return TankSpawnComplete{.uuid = e.uuid}; });
	Bind<BonusSpawnCompletedEvent>([](const auto& e) { return BonusSpawnComplete{.uuid = e.uuid}; });
	Bind<TierChangedEvent>([](const auto& e)
	{
		return TierChange{.who = e.who, .tier = e.tier, .uuid = e.uuid};
	});

	SubscribeStatistics();
	SubscribeBonus();
}

//NOTE: eleven distinct local types collapse onto one command - StatisticsType is the discriminator,
//so each line names its enum value and the fields that value actually carries.
void ReplicationPublisher::SubscribeStatistics()
{
	Bind<StatisticsBulletHitEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BulletHit,
								.author = e.author};
	});
	Bind<StatisticsTankHitEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::TankHit,
								.who = e.who,
								.author = e.author};
	});
	Bind<TankDiedEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::TankDied,
								.who = e.who,
								.author = e.author,
								.uuid = e.uuid};
	});
	Bind<BrickWallDiedEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BrickWallDied,
								.author = e.author};
	});
	Bind<SteelWallDiedEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::SteelWallDied,
								.author = e.author};
	});
	Bind<StatisticsBonusPickupEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BonusPickup,
								.author = e.author};
	});
	Bind<StatisticsBonusDestroyedEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BonusDestroyed,
								.author = e.author};
	});
	Bind<StatisticsBonusExpiredEvent>([](const auto&)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BonusExpired};
	});
}

void ReplicationPublisher::SubscribeBonus()
{
	Bind<BonusSpawnedEvent>([](const auto& e)
	{
		return BonusSpawn{.pos = e.pos, .bonusType = e.type, .uuid = e.uuid, .isSuper = e.isSuper};
	});
	Bind<BonusHelmetAppliedEvent>([](const auto& e)
	{
		return BonusStatus{.name = e.name, .bonusType = BonusType::Helmet, .isEnable = e.isActive};
	});
	Bind<BonusShipAppliedEvent>([](const auto& e)
	{
		return BonusStatus{.name = e.name, .bonusType = BonusType::Ship};
	});
	Bind<BonusTankAppliedEvent>([](const auto& e)
	{
		return BonusStatus{.author = e.author, .bonusType = BonusType::Tank};
	});
}
}//namespace network::commands
