#pragma once

#include "enums/ClientSignal.h"
#include "enums/Delivery.h"

namespace network::commands
{
struct SignalEvent final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	ClientSignal signal{};
};
}//namespace network::commands
