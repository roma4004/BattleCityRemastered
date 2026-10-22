#include "network/ReplicationApplier.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/StatisticsEvents.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/GameState.h"
#include "enums/InputSignal.h"
#include "enums/PortForwarding.h"
#include "enums/StatisticsType.h"
#include "network/commands/WorldSnapshot.h"
#include "utils/Log.h"
#include <algorithm>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace network
{
ReplicationApplier::ReplicationApplier(const std::shared_ptr<EventSystem>& events,
									   network::NetworkCommandQueue& queue)
	: _events{events}
	, _queue{queue} {}

void ReplicationApplier::Apply(const commands::AnyCommand& command)
{
	//NOTE: the lambda is std::visit's callable, so Emit stays a private overload set rather than a functor
	_queue.Enqueue([this, command] { std::visit([this](const auto& alternative) { Emit(alternative); }, command); });
}

void ReplicationApplier::Emit(const commands::PositionChange& command) const
{
	_events->EmitEvent(Key(command.uuid),
					   PosChangedEvent{.pos = command.pos, .dir = command.dir, .uuid = command.uuid});
}

void ReplicationApplier::Emit(const commands::TankShot& command) const
{
	const Author who{SeatFromWire(command.who)};
	_events->EmitEvent(Key(who), TankShotEvent{.who = who, .dir = command.dir, .bulletUuid = command.uuid});
}

void ReplicationApplier::Emit(const commands::HealthChange& command) const
{
	_events->EmitEvent(Key(command.uuid),
					   HealthChangedEvent{.health = command.health, .uuid = command.uuid});
}

void ReplicationApplier::Emit(const commands::TierChange& command) const
{
	_events->EmitEvent(Key(command.uuid),
					   TierChangedEvent{.tier = command.tier, .uuid = command.uuid});
}

void ReplicationApplier::Emit(const commands::TankGrew& command) const
{
	_events->EmitEvent(Key(command.uuid), TankGrewEvent{.rect = command.rect, .uuid = command.uuid});
}

void ReplicationApplier::Emit(const commands::Despawn& command) const
{
	_events->EmitEvent(Key(command.uuid),
					   DespawnedEvent{.uuid = command.uuid, .reason = command.reason});
}

void ReplicationApplier::Emit(const commands::RespawnTank& command) const
{
	_events->EmitEvent(TankRespawnedEvent{.type = command.tankType,
										  .model = command.model,
										  .tier = command.tier,
										  .uuid = command.uuid,
										  .pos = command.pos});
}

void ReplicationApplier::Emit(const commands::ObstacleSpawn& command) const
{
	_events->EmitEvent(ObstacleSpawnedEvent{.pos = command.pos, .type = command.obstacleType, .uuid = command.uuid});
}

void ReplicationApplier::Emit(const commands::TankSpawnComplete& command) const
{
	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = command.uuid});
}

void ReplicationApplier::Emit(const commands::TankSpawnMoved& command) const
{
	_events->EmitEvent(TankSpawnMovedEvent{.uuid = command.uuid, .pos = command.pos});
}

void ReplicationApplier::Emit(const commands::BonusSpawnComplete& command) const
{
	_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = command.uuid});
}

void ReplicationApplier::Emit(const commands::GameStateChange& command) const
{
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = command.state});
}

void ReplicationApplier::Emit(const commands::KeyStateChange& command) const
{
	if (command.action == InputSignal::PauseStatus)
	{
		_events->EmitEvent(SetPauseEvent{.isPaused = command.isPressed});
		return;
	}

	Log::Info("ReplicationApplier: unexpected signal " + std::to_string(static_cast<int>(command.action)));
}

//NOTE: one command back into eight local types by StatisticsType - the inverse of BindHostStatistics
void ReplicationApplier::Emit(const commands::StatisticsChange& command) const
{
	const Author who{SeatFromWire(command.who)};
	const Author author{SeatFromWire(command.author)};

	switch (command.statisticsType)
	{
		case StatisticsType::BulletHit:
			_events->EmitEvent(StatisticsBulletHitEvent{.author = author});
			break;
		case StatisticsType::TankHit:
			_events->EmitEvent(StatisticsTankHitEvent{.who = who, .author = author});
			break;
		case StatisticsType::TankDied:
			_events->EmitEvent(TankDiedEvent{.who = who, .uuid = command.uuid, .author = author});
			break;
		case StatisticsType::BrickWallDied:
			_events->EmitEvent(BrickWallDiedEvent{.author = author});
			break;
		case StatisticsType::SteelWallDied:
			_events->EmitEvent(SteelWallDiedEvent{.author = author});
			break;
		case StatisticsType::BonusPickup:
			_events->EmitEvent(StatisticsBonusPickupEvent{.author = author});
			break;
		case StatisticsType::BonusDestroyed:
			_events->EmitEvent(StatisticsBonusDestroyedEvent{.author = author});
			break;
		case StatisticsType::BonusExpired:
			_events->EmitEvent(StatisticsBonusExpiredEvent{});
			break;
	}
}

void ReplicationApplier::Emit(const commands::BonusSpawn& command) const
{
	//NOTE: the type indexes the bonus catalogue, so a wire byte outside the enum is stopped here
	if (!IsSpawnableBonus(command.bonusType))
	{
		Log::Error("ReplicationApplier: bonus type " + std::to_string(static_cast<int>(command.bonusType))
				   + " is not spawnable");

		return;
	}

	_events->EmitEvent(BonusSpawnedEvent{.pos = command.pos,
										 .type = command.bonusType,
										 .uuid = command.uuid,
										 .isSuper = command.isSuper});
}

void ReplicationApplier::Emit(const commands::SlotAssignment& command) const
{
	_events->EmitEvent(PlayerSlotAssignedEvent{.slot = command.slot, .match = command.match});
}

//NOTE: replaced, not patched - the reset first, then the phase before the field, since entering a match clears it
void ReplicationApplier::Emit(const commands::WorldSnapshot& command) const
{
	const auto isSpawnable = [](const BonusSpawnedEvent& bonus) { return IsSpawnableBonus(bonus.type); };
	if (!std::ranges::all_of(command.bonuses, isSpawnable) || !std::ranges::all_of(command.bonusSpawns, isSpawnable))
	{
		Log::Error("ReplicationApplier: world snapshot carries a bonus that is not spawnable, ignored");

		return;
	}

	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = command.phase});

	//NOTE: the field is sized first; a snapshot from before the host loaded a map carries zeros and sizes nothing
	if (command.map.cols != 0u && command.map.rows != 0u)
	{
		_events->EmitEvent(command.map);
	}

	_events->EmitEvent(WorldSnapshotReceivedEvent{.snapshot = command});
	_events->EmitEvent(SeatHoldersAnnouncedEvent{.holders = command.holders});
	_events->EmitEvent(AbsenceChangedEvent{.seats = command.absence});
	EmitEffects(command.effects);
	_events->EmitEvent(SetPauseEvent{.isPaused = command.phase == GameState::Paused});
}

void ReplicationApplier::Emit(const commands::AbsenceChange& command) const
{
	_events->EmitEvent(AbsenceChangedEvent{.seats = command.seats});
}

void ReplicationApplier::Emit(const commands::SeatHoldersChange& command) const
{
	_events->EmitEvent(SeatHoldersAnnouncedEvent{.holders = command.holders});
}

void ReplicationApplier::Emit(const commands::PortForwardingChange& command) const
{
	//NOTE: the lobby picks its words by the state, so a wire byte outside the enum is stopped where it enters
	if (command.state >= PortForwarding::lastId)
	{
		Log::Error("ReplicationApplier: port forwarding state " + std::to_string(static_cast<int>(command.state))
				   + " is unknown");

		return;
	}

	_events->EmitEvent(PortForwardingChangedEvent{.state = command.state, .host = command.host, .port = command.port});
}

void ReplicationApplier::Emit(const commands::BonusStatus& command) const
{
	//NOTE: only bonuses the client cannot see otherwise - a star or a caliber comes as TierChange, so is invalid here
	const Author author{SeatFromWire(command.author)};
	switch (command.bonusType)
	{
		case BonusType::Helmet:
			_events->EmitEvent(Key(author), BonusHelmetAppliedEvent{.author = author, .isActive = command.isEnable});
			return;
		case BonusType::Ship:
			_events->EmitEvent(Key(author), BonusShipAppliedEvent{.author = author});
			return;
		case BonusType::Tank:
			_events->EmitEvent(BonusTankAppliedEvent{.author = author});
			return;
		case BonusType::Star:
		case BonusType::Caliber:
		case BonusType::None:
		case BonusType::Timer:
		case BonusType::Grenade:
		case BonusType::Shovel:
		case BonusType::lastId:
			break;
	}

	Log::Error("ReplicationApplier: bonus type " + std::to_string(static_cast<int>(command.bonusType))
			   + " is not replicated");
}

void ReplicationApplier::Emit(const commands::BonusEffects& command) const { EmitEffects(command.effects); }

void ReplicationApplier::EmitEffects(std::vector<BonusEffectTimeLeft> effects) const
{
	const auto hasNoClock = [](const BonusEffectTimeLeft& effect)
	{
		return effect.type != BonusType::Timer && effect.type != BonusType::Helmet && effect.type != BonusType::Shovel;
	};
	if (std::erase_if(effects, hasNoClock) > 0u)
	{
		Log::Error("ReplicationApplier: a bonus that runs on no clock came as an effect");
	}

	std::ranges::for_each(effects, [](BonusEffectTimeLeft& effect)
	{
		effect.spared = SeatFromWire(effect.spared);
		if (Author* const seat{std::get_if<Author>(&effect.target)})
		{
			*seat = SeatFromWire(*seat);
		}
	});

	_events->EmitEvent(BonusEffectsChangedEvent{.effects = std::move(effects)});
}
}//namespace network
