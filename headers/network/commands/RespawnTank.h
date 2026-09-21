#pragma once

#include "geometry/Point.h"
#include "enums/Delivery.h"
#include "enums/TankType.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct RespawnTank final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	TankType tankType{};
	Uuid uuid{};
	FPoint pos{};
};
}//namespace network::commands
