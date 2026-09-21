#pragma once

#include "geometry/Point.h"
#include "enums/Delivery.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct PositionChange final
{
	static constexpr Delivery kDelivery{Delivery::Latest};

	FPoint pos{};
	Direction dir{};
	Uuid uuid{};
};
}//namespace network::commands
