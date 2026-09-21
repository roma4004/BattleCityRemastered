#pragma once

#include "enums/Delivery.h"
#include "enums/PlayerSlot.h"

namespace network::commands
{
struct SlotAssignment final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	PlayerSlot slot{};
};
}//namespace network::commands
