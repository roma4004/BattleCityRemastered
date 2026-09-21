#pragma once

#include "geometry/Point.h"
#include "enums/Delivery.h"
#include "enums/ObstacleType.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct ObstacleSpawn final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	FPoint pos{};
	ObstacleType obstacleType{};
	Uuid uuid{};
};
}//namespace network::commands
