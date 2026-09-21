#pragma once

#include "enums/Delivery.h"
#include "enums/DisconnectReason.h"

namespace network::commands
{
struct Disconnect final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	DisconnectReason reason{};
};
}//namespace network::commands
