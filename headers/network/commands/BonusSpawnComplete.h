#pragma once

#include "utils/Uuid.h"

namespace network::commands
{
struct BonusSpawnComplete final
{
	Uuid uuid{};
};
}//namespace network::commands
