#pragma once

#include "geometry/Point.h"
#include "enums/BonusType.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct BonusSpawn final
{
	FPoint pos{};
	BonusType bonusType{};
	Uuid uuid{};
	bool isSuper{};
};
}//namespace network::commands
