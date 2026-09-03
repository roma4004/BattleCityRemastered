#pragma once

#include "enums/Author.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct TankShot final
{
	Author who{};
	Direction dir{};
	Uuid uuid{};
};
}//namespace network::commands
