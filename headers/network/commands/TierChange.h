#pragma once

#include "enums/Delivery.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct TierChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	unsigned short tier{};
	Uuid uuid{};
};
}//namespace network::commands
