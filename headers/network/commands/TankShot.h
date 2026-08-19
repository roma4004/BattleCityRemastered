#pragma once

#include "enums/CommandType.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct TankShot final
{
	CommandType type{CommandType::TANK_SHOT};
	std::string who{};
	Direction dir{};
	Uuid uuid{};
};
}//namespace network::commands
