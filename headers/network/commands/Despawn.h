#pragma once

#include "enums/DespawnReason.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct Despawn final
{
	Uuid uuid{};
	DespawnReason reason{DespawnReason::None};
};
}//namespace network::commands
