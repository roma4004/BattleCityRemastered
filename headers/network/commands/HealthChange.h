#pragma once

#include "enums/CommandType.h"
#include "utils/Uuid.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct HealthChange final
{
	CommandType type{CommandType::HEALTH_CHANGE};
	std::string who{};
	int health{};
	Uuid uuid{};
};
}//namespace network::commands
