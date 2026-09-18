#pragma once

#include "geometry/Point.h"
#include "enums/TankType.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct RespawnTank final
{
	TankType tankType{};
	Uuid uuid{};
	FPoint pos{};
};
}//namespace network::commands
