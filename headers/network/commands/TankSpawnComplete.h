#pragma once

#include "utils/Uuid.h"

namespace network::commands
{
struct TankSpawnComplete final
{
	Uuid uuid{};
};
}//namespace network::commands
