#pragma once

#include "BonusSpawn.h"
#include "BonusSpawnComplete.h"
#include "BonusStatus.h"
#include "Despawn.h"
#include "Disconnect.h"
#include "GameStateChange.h"
#include "HealthChange.h"
#include "KeyStateChange.h"
#include "ObstacleSpawn.h"
#include "PositionChange.h"
#include "RespawnTank.h"
#include "SignalEvent.h"
#include "SlotAssignment.h"
#include "StatisticsChange.h"
#include "TankShot.h"
#include "TankSpawnComplete.h"
#include "TankSpawnMoved.h"
#include "TierChange.h"
#include "components/WorldSnapshot.h"
#include "enums/Delivery.h"
#include "utils/Uuid.h"
#include <concepts>
#include <variant>

namespace network::commands
{
//NOTE: every alternative is a plain wire DTO - their serialization lives in CommandSerialization.h
//NOTE: ser20 puts the alternative index on the wire - append only, inserting renumbers everything after
using AnyCommand = std::variant<
	BonusSpawn, BonusStatus, Despawn, GameStateChange, HealthChange,
	KeyStateChange, ObstacleSpawn, PositionChange, RespawnTank, SignalEvent, StatisticsChange, TankShot,
	TankSpawnComplete, Disconnect, BonusSpawnComplete, TierChange, SlotAssignment, WorldSnapshot,
	TankSpawnMoved>;

//NOTE: a Latest command is replaced per entity on its way out, so it has to say which entity that is
template<class CommandT>
concept LatestCommand = CommandT::kDelivery == Delivery::Latest && requires(const CommandT& command) {
	{ command.uuid } -> std::convertible_to<Uuid>;
};

//NOTE: a command without its own kDelivery does not compile here - there is no default to fall back on
template<class Variant>
inline constexpr bool kIsEveryDeliveryNamed{false};

template<class... CommandTs>
inline constexpr bool kIsEveryDeliveryNamed<std::variant<CommandTs...>>{
		(... && (CommandTs::kDelivery == Delivery::Reliable || LatestCommand<CommandTs>))};

static_assert(kIsEveryDeliveryNamed<AnyCommand>, "a Latest command must carry the uuid it is replaced by");
}//namespace network::commands
