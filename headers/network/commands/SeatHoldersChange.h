#pragma once

#include "enums/Delivery.h"
#include "enums/PlayerSlot.h"
#include <array>

namespace network::commands
{
struct SeatHoldersChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	std::array<SeatHolder, kSeatCount> holders{};
};
}//namespace network::commands
