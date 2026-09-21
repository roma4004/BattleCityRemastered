#pragma once

#include "enums/Delivery.h"
#include "enums/GameState.h"

namespace network::commands
{
struct GameStateChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	GameState state{};
};
}//namespace network::commands
