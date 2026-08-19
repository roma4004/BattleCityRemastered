#pragma once

#include "enums/CommandType.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct TankSpawnComplete final
{
	CommandType type{CommandType::TANK_SPAWN_COMPLETE};
	Uuid uuid{};
};
}//namespace network::commands
