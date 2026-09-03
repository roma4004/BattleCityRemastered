#pragma once

#include "utils/Uuid.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct HealthChange final
{
	std::string who{};
	int health{};
	Uuid uuid{};
};
}//namespace network::commands
