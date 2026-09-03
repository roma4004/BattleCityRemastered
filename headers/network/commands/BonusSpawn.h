#pragma once

#include "geometry/Point.h"
#include "enums/BonusType.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct BonusSpawn final
{
	FPoint pos{};
	BonusType bonusType{};
	Uuid uuid{};
	bool isSuper{};
};
}//namespace network::commands
