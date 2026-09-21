#pragma once

#include "enums/Delivery.h"
#include "enums/InputSignal.h"

namespace network::commands
{
struct KeyStateChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	InputSignal action{};
	bool isPressed{};
};
}//namespace network::commands
