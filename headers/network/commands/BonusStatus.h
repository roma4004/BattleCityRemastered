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
	//NOTE: the ship's only
	unsigned short charges{};
};
}//namespace network::commands
