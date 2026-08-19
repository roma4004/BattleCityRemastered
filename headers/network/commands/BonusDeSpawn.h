#pragma once

#include "enums/CommandType.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct BonusDeSpawn final
{
	CommandType type{CommandType::BONUS_DESPAWN};
	Uuid uuid{};
};
}//namespace network::commands
