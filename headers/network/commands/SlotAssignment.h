#pragma once

#include "enums/PlayerSlot.h"

namespace network::commands
{
struct SlotAssignment final
{
	PlayerSlot slot{};
};
}//namespace network::commands
