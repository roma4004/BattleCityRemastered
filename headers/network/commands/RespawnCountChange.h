#pragma once

#include "enums/Delivery.h"
#include "enums/RespawnGroup.h"

namespace network::commands
{
//NOTE: the count itself - the client counts nothing
struct RespawnCountChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	RespawnGroup group{};
	unsigned short respawnCount{};
};
}//namespace network::commands
