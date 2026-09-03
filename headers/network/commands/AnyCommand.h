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
#include "StatisticsChange.h"
#include "TankShot.h"
#include "TankSpawnComplete.h"
#include "TierChange.h"
#include <variant>

namespace network::commands
{
//NOTE: ser20 puts the alternative index on the wire - append only, inserting renumbers everything after
using AnyCommand = std::variant<
	BonusSpawn, BonusStatus, Despawn, GameStateChange, HealthChange,
	KeyStateChange, ObstacleSpawn, PositionChange, RespawnTank, SignalEvent, StatisticsChange, TankShot,
	TankSpawnComplete, Disconnect, BonusSpawnComplete, TierChange>;
}//namespace network::commands
