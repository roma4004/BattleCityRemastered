#pragma once

#include "utils/Uuid.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct TierChange final
{
	std::string who{};
	unsigned short tier{};
	Uuid uuid{};
};
}//namespace network::commands
