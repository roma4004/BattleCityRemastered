#pragma once

#include "utils/Uuid.h"

namespace network::commands
{
struct TierChange final
{
	unsigned short tier{};
	Uuid uuid{};
};
}//namespace network::commands
