#pragma once

#include "enums/Delivery.h"
#include "geometry/Point.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: reliable, unlike positions - the client lands the tank on the last square, so no shove may be dropped
struct TankSpawnMoved final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	Uuid uuid{};
	FPoint pos{};
};
}//namespace network::commands
