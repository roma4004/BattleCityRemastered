#pragma once

#include "enums/Delivery.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct HealthChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	int health{};
	Uuid uuid{};
};
}//namespace network::commands
