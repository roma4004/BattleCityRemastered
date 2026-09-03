#pragma once

#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct HealthChange final
{
	int health{};
	Uuid uuid{};
};
}//namespace network::commands
