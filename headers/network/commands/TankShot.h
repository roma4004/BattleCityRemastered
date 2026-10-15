#pragma once

#include "enums/Author.h"
#include "enums/Delivery.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct TankShot final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	Author who{};
	Direction dir{};
	Uuid uuid{};
};
}//namespace network::commands
