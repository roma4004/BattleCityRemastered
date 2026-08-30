#pragma once

#include "enums/CommandType.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct BonusSpawnComplete final
{
	CommandType type{CommandType::BONUS_SPAWN_COMPLETE};
	Uuid uuid{};
};
}//namespace network::commands
