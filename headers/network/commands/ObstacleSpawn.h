#pragma once

#include "geometry/Point.h"
#include "enums/ObstacleType.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct ObstacleSpawn final
{
	FPoint pos{};
	ObstacleType obstacleType{};
	Uuid uuid{};
};
}//namespace network::commands
