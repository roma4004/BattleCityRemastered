#pragma once

#include "BonusDeSpawn.h"
#include "BonusSpawn.h"
#include "BonusStatus.h"
#include "Disconnect.h"
#include "Dispose.h"
#include "FortressChange.h"
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
#include "enums/CommandType.h"
#include <variant>

namespace network::commands
{
//NOTE: ser20 puts the alternative index on the wire - append only, inserting renumbers everything after
using AnyCommand = std::variant<
	BonusDeSpawn, BonusSpawn, BonusStatus, Dispose, FortressChange, GameStateChange, HealthChange,
	KeyStateChange, ObstacleSpawn, PositionChange, RespawnTank, SignalEvent, StatisticsChange, TankShot,
	TankSpawnComplete, Disconnect>;

inline CommandType GetCommandType(const AnyCommand& command)
{
	return std::visit([](const auto& cmd) { return cmd.type; }, command);
}

}//namespace network::commands
