#pragma once

#include "geometry/Point.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct PositionChange final
{
	std::string who{};
	FPoint pos{};
	Direction dir{};
	Uuid uuid{};
};
}//namespace network::commands
