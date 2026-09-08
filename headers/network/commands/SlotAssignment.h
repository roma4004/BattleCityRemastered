#pragma once

#include "enums/PlayerSlot.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct SlotAssignment final
{
	PlayerSlot slot{};
};
}//namespace network::commands
