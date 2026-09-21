#pragma once

#include "enums/Author.h"
#include "enums/Delivery.h"
#include "enums/StatisticsType.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct StatisticsChange final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	StatisticsType statisticsType{};
	//NOTE: only the tank facts carry it; the receiver picks the counter
	Author who{};
	Author author{};
	//NOTE: only TankDied carries it - a name is not identity
	Uuid uuid{};
};
}//namespace network::commands
