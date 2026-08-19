#pragma once

#include "enums/CommandType.h"
#include "enums/FortressState.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct FortressChange final
{
	CommandType type{CommandType::FORTRESS_CHANGE};
	FortressState state{};
	Uuid uuid{};
};
}//namespace network::commands
