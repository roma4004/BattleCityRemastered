#pragma once

#include "geometry/Point.h"
#include "enums/BonusType.h"
#include "enums/Delivery.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct BonusSpawn final
{
	static constexpr Delivery kDelivery{Delivery::Reliable};

	FPoint pos{};
	BonusType bonusType{};
	Uuid uuid{};
	bool isSuper{};
};
}//namespace network::commands
