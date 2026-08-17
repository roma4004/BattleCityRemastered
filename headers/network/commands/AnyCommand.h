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
//NOTE: no name from it appears here - it teaches ser20 how to (de)serialise a variant, found via ADL
#include <ser20/types/variant.hpp>
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
	return std::visit([](const auto& cmd) { return cmd.GetType(); }, command);
}

inline const char* GetClassNameW(const AnyCommand& command)
{
	return std::visit([](const auto& cmd) { return cmd.GetClassNameW(); }, command);
}
}//namespace network::commands
