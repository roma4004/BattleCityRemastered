#pragma once

#include "utils/Uuid.h"

namespace network::commands
{
struct HealthChange final
{
	int health{};
	Uuid uuid{};
};
}//namespace network::commands
