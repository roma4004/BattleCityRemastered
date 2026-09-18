#pragma once

#include "geometry/Point.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct PositionChange final
{
	FPoint pos{};
	Direction dir{};
	Uuid uuid{};
};
}//namespace network::commands
