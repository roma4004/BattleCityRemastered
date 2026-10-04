#pragma once

#include "enums/Delivery.h"
#include "enums/PortForwarding.h"
#include <cstdint>
#include <string>

namespace network::commands
{
struct PortForwardingChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	PortForwarding state{};
	//NOTE: the internet's address of the router, once the port is open
	std::string host{};
	//NOTE: the server's port, the same number outside and in
	std::uint16_t port{};
};
}//namespace network::commands
