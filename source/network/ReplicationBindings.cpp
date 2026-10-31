#include "network/ReplicationBindings.h"
#include "network/ReplicationPublisher.h"
#include "enums/Absence.h"
#include "enums/ClientSignal.h"
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

namespace network
{
namespace
{
//NOTE: eight local types onto one command - each line names its StatisticsType and the fields it carries
void BindHostStatistics(ReplicationPublisher& out)
{
	out.Bind<StatisticsBulletHitEvent>([](const auto& e)
	{
		return commands::StatisticsChange{.statisticsType = StatisticsType::BulletHit,
								.author = e.author};
	});
	out.Bind<StatisticsTankHitEvent>([](const auto& e)
	{
		return commands::StatisticsChange{.statisticsType = StatisticsType::TankHit,
								.who = e.who,
								.author = e.author};
	});
	out.Bind<TankDiedEvent>([](const auto& e)
	{
		return commands::StatisticsChange{.statisticsType = StatisticsType::TankDied,
								.who = e.who,
								.author = e.author,
								.uuid = e.uuid};
	});
	out.Bind<BrickWallDiedEvent>([](const auto& e)
	{
		return commands::StatisticsChange{.statisticsType = StatisticsType::BrickWallDied,
								.author = e.author};
	});
	out.Bind<SteelWallDiedEvent>([](const auto& e)
	{
		return commands::StatisticsChange{.statisticsType = StatisticsType::SteelWallDied,
								.author = e.author};
	});
	out.Bind<StatisticsBonusPickupEvent>([](const auto& e)
	{
		return commands::StatisticsChange{.statisticsType = StatisticsType::BonusPickup,
								.author = e.author};
	});
	out.Bind<StatisticsBonusDestroyedEvent>([](const auto& e)
	{
		return commands::StatisticsChange{.statisticsType = StatisticsType::BonusDestroyed,
								.author = e.author};
	});
	out.Bind<StatisticsBonusExpiredEvent>([](const auto&)
	{
		return commands::StatisticsChange{.statisticsType = StatisticsType::BonusExpired};
	});
}

void BindHostBonus(ReplicationPublisher& out)
{
	out.Bind<BonusSpawnedEvent>([](const auto& e)
	{
		return commands::BonusSpawn{.pos = e.pos, .bonusType = e.type, .uuid = e.uuid, .isSuper = e.isSuper};
	});
	out.Bind<BonusHelmetAppliedEvent>([](const auto& e)
	{
		return commands::BonusStatus{.author = e.author, .bonusType = BonusType::Helmet, .isEnable = e.isActive};
	});
	out.Bind<BonusShipAppliedEvent>([](const auto& e)
	{
		return commands::BonusStatus{.author = e.author,
									 .bonusType = BonusType::Ship,
									 .isEnable = e.isActive,
									 .charges = e.charges};
	});
	out.Bind<RespawnCountChangedToEvent>([](const auto& e)
	{
		return commands::RespawnCountChange{.group = e.group, .respawnCount = e.respawnCount};
	});
	out.Bind<BonusEffectsChangedEvent>([](const auto& e) { return commands::BonusEffects{.effects = e.effects}; });
}

[[nodiscard]] ClientSignal SignalOf(const AbsenceChoice choice) noexcept
{
	switch (choice)
	{
		case AbsenceChoice::Continue:
			return ClientSignal::AbsenceContinue;
		case AbsenceChoice::Bot:
			return ClientSignal::AbsenceBot;
	}

	return ClientSignal::AbsenceContinue;
}

template<class EventT>
[[nodiscard]] EventSubscription BindKey(ReplicationPublisher& out, EventSystem& events, const InputChannel channel,
										const InputSignal action)
{
	return events.AddListener(Key(channel), [&out, action](const EventT& event)
	{
		out.Publish(commands::KeyStateChange{.action = action, .isPressed = event.isPressed});
	});
}
}//namespace
void BindHostReplication(ReplicationPublisher& out)
{
	out.Bind<PauseStatusEvent>([](const auto& e)
	{
		return commands::KeyStateChange{.action = InputSignal::PauseStatus, .isPressed = e.isPaused};
	});
	out.Bind<GameStateChangedToEvent>([](const auto& e) { return commands::GameStateChange{.state = e.state}; });
	out.Bind<AbsenceChangedEvent>([](const auto& e) { return commands::AbsenceChange{.seats = e.seats}; });
	out.Bind<SeatHoldersAnnouncedEvent>([](const auto& e)
	{
		return commands::SeatHoldersChange{.holders = e.holders};
	});

	out.Bind<PosChangedEvent>([](const auto& e)
	{
		return commands::PositionChange{.pos = e.pos, .dir = e.dir, .uuid = e.uuid};
	});
	out.Bind<TankShotEvent>([](const auto& e)
	{
		return commands::TankShot{.who = e.who, .dir = e.dir, .uuid = e.bulletUuid};
	});
	out.Bind<HealthChangedEvent>([](const auto& e)
	{
		return commands::HealthChange{.health = e.health, .uuid = e.uuid};
	});
	out.Bind<DespawnedEvent>([](const auto& e)
	{
		return commands::Despawn{.uuid = e.uuid, .reason = e.reason};
	});
	out.Bind<TankRespawnedEvent>([](const auto& e)
	{
		return commands::RespawnTank{
				.tankType = e.type, .model = e.model, .tier = e.tier, .uuid = e.uuid, .pos = e.pos};
	});
	out.Bind<ObstacleSpawnedEvent>([](const auto& e)
	{
		return commands::ObstacleSpawn{.pos = e.pos, .obstacleType = e.type, .uuid = e.uuid};
	});
	out.Bind<TankSpawnCompletedEvent>([](const auto& e) { return commands::TankSpawnComplete{.uuid = e.uuid}; });
	out.Bind<TankSpawnMovedEvent>([](const auto& e)
	{
		return commands::TankSpawnMoved{.uuid = e.uuid, .pos = e.pos};
	});
	out.Bind<BonusSpawnCompletedEvent>([](const auto& e) { return commands::BonusSpawnComplete{.uuid = e.uuid}; });
	out.Bind<TierChangedEvent>([](const auto& e)
	{
		return commands::TierChange{.tier = e.tier, .uuid = e.uuid};
	});
	out.Bind<TankGrewEvent>([](const auto& e) { return commands::TankGrew{.rect = e.rect, .uuid = e.uuid}; });

	BindHostStatistics(out);
	BindHostBonus(out);
}

void BindClientReplication(ReplicationPublisher& out)
{
	out.Bind<ClientOutReadyToPlayEvent>([](const auto&)
	{
		return commands::SignalEvent{.signal = ClientSignal::ReadyToPlay};
	});
	out.Bind<ClientOutRestartMatchEvent>([](const auto&)
	{
		return commands::SignalEvent{.signal = ClientSignal::RestartMatch};
	});
	out.Bind<NextLevelRequestedEvent>([](const auto&)
	{
		return commands::SignalEvent{.signal = ClientSignal::NextLevel};
	});
	out.Bind<WorldSnapshotReceivedEvent>([](const auto&)
	{
		return commands::SignalEvent{.signal = ClientSignal::WorldSynced};
	});
	out.Bind<AbsenceChosenEvent>([](const auto& e) { return commands::SignalEvent{.signal = SignalOf(e.choice)}; });
	out.Bind<PauseRequestedEvent>([](const auto& e)
	{
		return commands::KeyStateChange{.action = InputSignal::PauseRequest, .isPressed = e.isPaused};
	});
}

//NOTE: forwards the five keys of the slot's local half; the server takes the seat from the session they came on
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
}//namespace network
