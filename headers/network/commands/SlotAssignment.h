#pragma once

#include "components/MatchSettings.h"
#include "enums/Delivery.h"
#include "enums/PlayerSlot.h"

namespace network::commands
{
struct SlotAssignment final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	PlayerSlot slot{};
	//NOTE: the match the server was started for - its seat count, and what the lobby shows
	MatchSettings match{};
};
}//namespace network::commands
