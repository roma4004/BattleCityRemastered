#include "network/ReplicationBindings.h"
#include "network/ReplicationPublisher.h"
#include "enums/InputChannel.h"
#include "enums/InputSignal.h"
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
//NOTE: eleven local types collapse onto one command - each line names the StatisticsType that tells
//them apart and the fields that value carries
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

template<class EventT>
[[nodiscard]] EventSubscription BindKey(ReplicationPublisher& out, EventSystem& events, const InputChannel channel,
										const InputSignal action)
{
	return events.AddListener(Key(channel), [&out, action](const EventT& event)
	{
		out.Publish(KeyStateChange{.action = action, .isPressed = event.isPressed});
	});
}
}//namespace
void BindHostReplication(ReplicationPublisher& out)
{
	out.Bind<PauseStatusEvent>([](const auto& e)
	{
		return KeyStateChange{.action = InputSignal::PauseStatus, .isPressed = e.isPaused};
	});
	out.Bind<GameStateChangedToEvent>([](const auto& e) { return GameStateChange{.state = e.state}; });

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

void BindClientReplication(ReplicationPublisher& out)
{
	out.Bind<ClientOutReadyToPlayEvent>([](const auto&)
	{
		return SignalEvent{.signal = ClientSignal::ReadyToPlay};
	});
	out.Bind<ClientOutRestartMatchEvent>([](const auto&)
	{
		return SignalEvent{.signal = ClientSignal::RestartMatch};
	});
	out.Bind<PauseRequestedEvent>([](const auto& e)
	{
		return KeyStateChange{.action = InputSignal::PauseReleased, .isPressed = e.isPaused};
	});
}

//NOTE: forwards the five keys of the half that the server's slot maps to locally. The command names
//the key, and the server takes the seat from the session it arrived on
std::vector<EventSubscription> BindClientInput(ReplicationPublisher& out, EventSystem& events, const PlayerSlot slot)
{
	const InputChannel channel{LocalInput(slot)};

	std::vector<EventSubscription> subs{};
	subs.reserve(5u);
	subs.push_back(BindKey<MoveUpEvent>(out, events, channel, InputSignal::MoveUp));
	subs.push_back(BindKey<MoveLeftEvent>(out, events, channel, InputSignal::MoveLeft));
	subs.push_back(BindKey<MoveDownEvent>(out, events, channel, InputSignal::MoveDown));
	subs.push_back(BindKey<MoveRightEvent>(out, events, channel, InputSignal::MoveRight));
	subs.push_back(BindKey<FireEvent>(out, events, channel, InputSignal::Fire));

	return subs;
}
}//namespace network::commands
