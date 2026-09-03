#pragma once

#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct TierChange final
{
	unsigned short tier{};
	Uuid uuid{};
};
}//namespace network::commands
