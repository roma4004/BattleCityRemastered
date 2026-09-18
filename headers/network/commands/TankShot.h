#pragma once

#include "enums/Author.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"

namespace network::commands
{
struct TankShot final
{
	Author who{};
	Direction dir{};
	Uuid uuid{};
};
}//namespace network::commands
