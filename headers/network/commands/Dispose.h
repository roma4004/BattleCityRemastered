#pragma once

#include "enums/CommandType.h"
#include "utils/Uuid.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct Dispose final
{
	CommandType type{CommandType::DISPOSE};
	std::string who{};
	Uuid uuid{};
};
}//namespace network::commands
