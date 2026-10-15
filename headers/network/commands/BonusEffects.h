#pragma once

#include "components/events/BonusPickupEvents.h"
#include "enums/Delivery.h"
#include <vector>

namespace network::commands
{
struct BonusEffects final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	std::vector<BonusEffectTimeLeft> effects{};
};
}//namespace network::commands
