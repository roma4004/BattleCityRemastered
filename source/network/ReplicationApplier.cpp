#include "network/ReplicationApplier.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/WorldSnapshot.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/GameState.h"
#include "enums/InputSignal.h"
#include "enums/StatisticsType.h"
#include "utils/Log.h"
#include <algorithm>
#include <string>
#include <variant>

namespace network::commands
{
ReplicationApplier::ReplicationApplier(const std::shared_ptr<EventSystem>& events,
									   network::NetworkCommandQueue& queue)
	: _events{events}
	, _queue{queue} {}

void ReplicationApplier::Apply(const AnyCommand& command)
{
	//NOTE: the lambda is the callable std::visit wants, so Emit stays a named private overload set
	//rather than the class pretending to be a functor
	_queue.Enqueue([this, command] { std::visit([this](const auto& alternative) { Emit(alternative); }, command); });
}

void ReplicationApplier::Emit(const PositionChange& command) const
{
	_events->EmitEvent(Key(command.uuid),
					   PosChangedEvent{.pos = command.pos, .dir = command.dir, .uuid = command.uuid});
}

void ReplicationApplier::Emit(const TankShot& command) const
{
	const Author who{SeatFromWire(command.who)};
	_events->EmitEvent(Key(who), TankShotEvent{.who = who,
											   .dir = command.dir,
											   .bulletUuid = command.uuid,
											   .damage = command.damage});
}

void ReplicationApplier::Emit(const HealthChange& command) const
{
	_events->EmitEvent(Key(command.uuid),
					   HealthChangedEvent{.health = command.health, .uuid = command.uuid});
}

void ReplicationApplier::Emit(const TierChange& command) const
{
	_events->EmitEvent(Key(command.uuid),
					   TierChangedEvent{.tier = command.tier, .uuid = command.uuid});
}

void ReplicationApplier::Emit(const Despawn& command) const
{
	_events->EmitEvent(Key(command.uuid),
					   DespawnedEvent{.uuid = command.uuid, .reason = command.reason});
}

void ReplicationApplier::Emit(const RespawnTank& command) const
{
	_events->EmitEvent(TankRespawnedEvent{.type = command.tankType,
										  .model = command.model,
										  .uuid = command.uuid,
										  .pos = command.pos});
}

void ReplicationApplier::Emit(const ObstacleSpawn& command) const
{
	_events->EmitEvent(ObstacleSpawnedEvent{.pos = command.pos, .type = command.obstacleType, .uuid = command.uuid});
}

void ReplicationApplier::Emit(const TankSpawnComplete& command) const
{
	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = command.uuid});
}

void ReplicationApplier::Emit(const TankSpawnMoved& command) const
{
	_events->EmitEvent(TankSpawnMovedEvent{.uuid = command.uuid, .pos = command.pos});
}

void ReplicationApplier::Emit(const BonusSpawnComplete& command) const
{
	_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = command.uuid});
}

void ReplicationApplier::Emit(const GameStateChange& command) const
{
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = command.state});
}

void ReplicationApplier::Emit(const KeyStateChange& command) const
{
	if (command.action == InputSignal::PauseStatus)
	{
		_events->EmitEvent(SetPauseEvent{.isPaused = command.isPressed});
		return;
	}

	Log::Info("ReplicationApplier: unexpected signal " + std::to_string(static_cast<int>(command.action)));
}

//NOTE: one command fans back out into eight local types - StatisticsType is the discriminator the
//host wrote, so this is the exact inverse of the statistics half of the host bindings
void ReplicationApplier::Emit(const StatisticsChange& command) const
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

void ReplicationApplier::Emit(const BonusSpawn& command) const
{
	//NOTE: the type indexes the bonus catalogue further down, so a wire byte outside the enum
	//would read past its end - checked here, where the untrusted value enters
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

void ReplicationApplier::Emit(const SlotAssignment& command) const
{
	_events->EmitEvent(PlayerSlotAssignedEvent{.slot = command.slot, .match = command.match});
}

//NOTE: the mirror is replaced, not patched - the reset first, then the phase before the field, since
//entering a match clears it
void ReplicationApplier::Emit(const WorldSnapshot& command) const
{
	const auto isSpawnable = [](const BonusSpawnedEvent& bonus) { return IsSpawnableBonus(bonus.type); };
	if (!std::ranges::all_of(command.bonuses, isSpawnable) || !std::ranges::all_of(command.bonusSpawns, isSpawnable))
	{
		Log::Error("ReplicationApplier: world snapshot carries a bonus that is not spawnable, ignored");

		return;
	}

	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = command.phase});

	//NOTE: the field is sized before anything is put on it; a snapshot taken before the host loaded
	//a map carries zeros, and no size at all is better than a field of none
	if (command.map.cols != 0u && command.map.rows != 0u)
	{
		_events->EmitEvent(command.map);
	}

	_events->EmitEvent(WorldSnapshotReceivedEvent{.snapshot = command});
	_events->EmitEvent(AbsenceChangedEvent{.seats = command.absence});
	_events->EmitEvent(SetPauseEvent{.isPaused = command.phase == GameState::Paused});
}

void ReplicationApplier::Emit(const AbsenceChange& command) const
{
	_events->EmitEvent(AbsenceChangedEvent{.seats = command.seats});
}

void ReplicationApplier::Emit(const BonusStatus& command) const
{
	//NOTE: only the bonuses whose effect the client cannot see any other way arrive here - a star and a
	//caliber land as a TierChange, so either of them is as wrong as a byte outside the enum
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
}//namespace network::commands
