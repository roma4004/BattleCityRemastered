#pragma once

#include "enums/Absence.h"
#include "enums/Delivery.h"
#include "enums/PlayerSlot.h"
#include <array>

namespace network::commands
{
struct AbsenceChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	std::array<Absence, kSeatCount> seats{};
};
}//namespace network::commands
