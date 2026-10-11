#pragma once

#include "geometry/Point.h"
#include "enums/Delivery.h"
#include "enums/TankModel.h"
#include "enums/TankType.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct RespawnTank final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	TankType tankType{};
	TankModel model{};
	unsigned short tier{1u};
	Uuid uuid{};
	FPoint pos{};
};
}//namespace network::commands
