#pragma once

#include "AbsenceChange.h"
#include "BonusEffects.h"
#include "BonusSpawn.h"
#include "BonusSpawnComplete.h"
#include "BonusStatus.h"
#include "Despawn.h"
#include "Disconnect.h"
#include "GameStateChange.h"
#include "HealthChange.h"
#include "KeyStateChange.h"
#include "ObstacleSpawn.h"
#include "PortForwardingChange.h"
#include "PositionChange.h"
#include "RespawnCountChange.h"
#include "RespawnTank.h"
#include "SeatHoldersChange.h"
#include "ShareRecipientChange.h"
#include "SignalEvent.h"
#include "SlotAssignment.h"
#include "StatisticsChange.h"
#include "TankGrew.h"
#include "TankShot.h"
#include "TankSpawnComplete.h"
#include "TankSpawnMoved.h"
#include "TierChange.h"
#include "WorldSnapshot.h"
#include "enums/Delivery.h"
#include "utils/Uuid.h"
#include <concepts>
#include <variant>

namespace network::commands
{
//NOTE: a command about one entity - the uuid names it
template<class CommandT>
concept EntityCommand = requires(const CommandT& command) { { command.uuid } -> std::convertible_to<Uuid>; };

//NOTE: a newer value replaces the older one of the same entity, newer by datagram seq - the uuid says which entity
template<class CommandT>
concept LatestCommand = CommandT::kDelivery == Delivery::Latest && EntityCommand<CommandT>;

//NOTE: without this check a command marked Latest with no uuid would be sent reliably by ReplicationPublisher, no error
template<class CommandT>
concept Command = CommandT::kDelivery == Delivery::Reliable || LatestCommand<CommandT>;

//NOTE: every command listed in it has to satisfy Command, or the build stops here - std::variant alone takes any type
template<Command... CommandTs>
using CommandVariant = std::variant<CommandTs...>;

//NOTE: ser20 puts the alternative index on the wire - append only, inserting renumbers everything after
using AnyCommand = CommandVariant<
	BonusSpawn, BonusStatus, Despawn, GameStateChange, HealthChange,
	KeyStateChange, ObstacleSpawn, PositionChange, RespawnTank, SignalEvent, StatisticsChange, TankShot,
	TankSpawnComplete, Disconnect, BonusSpawnComplete, TierChange, SlotAssignment, WorldSnapshot,
	TankSpawnMoved, AbsenceChange, PortForwardingChange, SeatHoldersChange, TankGrew, BonusEffects,
	RespawnCountChange, ShareRecipientChange>;
}//namespace network::commands
