#pragma once

#include "enums/Delivery.h"
#include "enums/DespawnReason.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct Despawn final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	Uuid uuid{};
	DespawnReason reason{DespawnReason::None};
};
}//namespace network::commands
