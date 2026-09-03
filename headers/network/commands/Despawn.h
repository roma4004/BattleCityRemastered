#pragma once

#include "enums/DespawnReason.h"
#include "utils/Uuid.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct Despawn final
{
	std::string who{};
	Uuid uuid{};
	DespawnReason reason{DespawnReason::None};
};
}//namespace network::commands
