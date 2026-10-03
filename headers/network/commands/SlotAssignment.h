#pragma once

#include "enums/Delivery.h"
#include "enums/PlayerSlot.h"
#include <cstdint>

namespace network::commands
{
struct SlotAssignment final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	PlayerSlot slot{};
	std::uint8_t seatCount{};
};
}//namespace network::commands
