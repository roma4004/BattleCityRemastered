#pragma once

#include "enums/Delivery.h"
#include "enums/PlayerSlot.h"
#include <optional>

namespace network::commands
{
//NOTE: the seat itself - the client counts nothing
struct ShareRecipientChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	std::optional<PlayerSlot> seat{};
};
}//namespace network::commands
