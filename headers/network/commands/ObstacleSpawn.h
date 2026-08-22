#pragma once

#include "geometry/Point.h"
#include "enums/CommandType.h"
#include "enums/ObstacleType.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct ObstacleSpawn final
{
	CommandType type{CommandType::OBSTACLE_SPAWN};
	FPoint pos{};
	ObstacleType obstacleType{};
	Uuid uuid{};
};
}//namespace network::commands
