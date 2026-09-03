#pragma once

#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct BonusSpawnComplete final
{
	Uuid uuid{};
};
}//namespace network::commands
