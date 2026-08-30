#pragma once

#include "enums/CommandType.h"
#include "utils/Uuid.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct TierChange final
{
	CommandType type{CommandType::TIER_CHANGE};
	std::string who{};
	unsigned short tier{};
	Uuid uuid{};
};
}//namespace network::commands
