#pragma once

#include "geometry/Point.h"
#include "enums/BonusType.h"
#include "enums/CommandType.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct BonusSpawn final
{
	CommandType type{CommandType::BONUS_SPAWN};
	FPoint pos{};
	BonusType bonusType{};
	Uuid uuid{};
};
}//namespace network::commands
