#include "network/ReplicationBindings.h"
#include "network/ReplicationPublisher.h"
#include "enums/InputChannel.h"
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

namespace network::commands
{
namespace
{
void BindHostStatistics(ReplicationPublisher& out)
{
	out.Bind<StatisticsBulletHitEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BulletHit,
								.author = e.author};
	});
	out.Bind<StatisticsTankHitEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::TankHit,
								.who = e.who,
								.author = e.author};
	});
	out.Bind<TankDiedEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::TankDied,
								.who = e.who,
								.author = e.author,
								.uuid = e.uuid};
	});
	out.Bind<BrickWallDiedEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BrickWallDied,
								.author = e.author};
	});
	out.Bind<SteelWallDiedEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::SteelWallDied,
								.author = e.author};
	});
	out.Bind<StatisticsBonusPickupEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BonusPickup,
								.author = e.author};
	});
	out.Bind<StatisticsBonusDestroyedEvent>([](const auto& e)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BonusDestroyed,
								.author = e.author};
	});
	out.Bind<StatisticsBonusExpiredEvent>([](const auto&)
	{
		return StatisticsChange{.statisticsType = StatisticsType::BonusExpired};
	});
}

void BindHostBonus(ReplicationPublisher& out)
{
	out.Bind<BonusSpawnedEvent>([](const auto& e)
	{
		return BonusSpawn{.pos = e.pos, .bonusType = e.type, .uuid = e.uuid, .isSuper = e.isSuper};
	});
	out.Bind<BonusHelmetAppliedEvent>([](const auto& e)
	{
		return BonusStatus{.author = e.author, .bonusType = BonusType::Helmet, .isEnable = e.isActive};
	});
	out.Bind<BonusShipAppliedEvent>([](const auto& e)
	{
		return BonusStatus{.author = e.author, .bonusType = BonusType::Ship};
	});
	out.Bind<BonusTankAppliedEvent>([](const auto& e)
	{
		return BonusStatus{.author = e.author, .bonusType = BonusType::Tank};
	});
}
}//namespace
void BindHostReplication(ReplicationPublisher& out)
{
	out.Bind<PauseStatusEvent>([](const auto& e)
	{
		return KeyStateChange{.tag = PlayerTag::None,
							  .action = InputSignal::PauseStatus,
							  .isPressed = e.isPaused};
	});
	out.Bind<GameFinishedEvent>([](const auto& e) { return GameStateChange{.state = e.state}; });

	out.Bind<PosChangedEvent>([](const auto& e)
	{
		return PositionChange{.pos = e.pos, .dir = e.dir, .uuid = e.uuid};
	});
	out.Bind<TankShotEvent>([](const auto& e) { return TankShot{.who = e.who, .dir = e.dir, .uuid = e.bulletUuid}; });
	out.Bind<HealthChangedEvent>([](const auto& e)
	{
		return HealthChange{.health = e.health, .uuid = e.uuid};
	});
	out.Bind<DespawnedEvent>([](const auto& e)
	{
		return Despawn{.uuid = e.uuid, .reason = e.reason};
	});
	out.Bind<TankRespawnedEvent>([](const auto& e)
	{
		return RespawnTank{.tankType = e.type, .uuid = e.uuid, .pos = e.pos};
	});
	out.Bind<ObstacleSpawnedEvent>([](const auto& e)
	{
		return ObstacleSpawn{.pos = e.pos, .obstacleType = e.type, .uuid = e.uuid};
	});
	out.Bind<TankSpawnCompletedEvent>([](const auto& e) { return TankSpawnComplete{.uuid = e.uuid}; });
	out.Bind<BonusSpawnCompletedEvent>([](const auto& e) { return BonusSpawnComplete{.uuid = e.uuid}; });
	out.Bind<TierChangedEvent>([](const auto& e)
	{
		return TierChange{.tier = e.tier, .uuid = e.uuid};
	});

	BindHostStatistics(out);
	BindHostBonus(out);
}

//NOTE: eleven distinct local types collapse onto one command - StatisticsType is the discriminator,
//so each line names its enum value and the fields that value actually carries.

void BindClientReplication(ReplicationPublisher& out)
{
	//NOTE: a keyboard half belongs to a seat, not to a machine - this process is player two, so it
	//takes the arrows like a second player anywhere else. Tab swaps the halves locally for whoever
	//would rather drive that seat with WASD
	constexpr InputChannel channel{InputChannel::LocalP2};

	const auto keyState = [](const InputSignal action)
	{
		return [action](const auto& e)
		{
			return KeyStateChange{.tag = PlayerTag::P2, .action = action, .isPressed = e.isPressed};
		};
	};

	out.Bind<MoveUpEvent>(Key(channel), keyState(InputSignal::MoveUp));
	out.Bind<MoveLeftEvent>(Key(channel), keyState(InputSignal::MoveLeft));
	out.Bind<MoveDownEvent>(Key(channel), keyState(InputSignal::MoveDown));
	out.Bind<MoveRightEvent>(Key(channel), keyState(InputSignal::MoveRight));
	out.Bind<FireEvent>(Key(channel), keyState(InputSignal::Fire));

	out.Bind<ClientOutReadyToPlayEvent>([](const auto&)
	{
		return SignalEvent{.signal = ClientSignal::ReadyToPlay};
	});
	out.Bind<PauseRequestedEvent>([](const auto& e)
	{
		return KeyStateChange{.tag = PlayerTag::None,
							  .action = InputSignal::PauseReleased,
							  .isPressed = e.isPaused};
	});
}
}//namespace network::commands
