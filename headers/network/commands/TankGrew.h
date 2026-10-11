#pragma once

#include "geometry/ObjRectangle.h"
#include "enums/Delivery.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct TankGrew final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	ObjRectangle rect{};
	Uuid uuid{};
};
}//namespace network::commands
