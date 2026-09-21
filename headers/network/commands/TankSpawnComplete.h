#pragma once

#include "enums/Delivery.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct TankSpawnComplete final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	Uuid uuid{};
};
}//namespace network::commands
