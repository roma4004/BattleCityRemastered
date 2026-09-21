#pragma once

#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/Delivery.h"

namespace network::commands
{
struct BonusStatus final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	Author author{};
	BonusType bonusType{};
	bool isEnable{};
};
}//namespace network::commands
