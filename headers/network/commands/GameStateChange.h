#pragma once

#include "enums/GameState.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct GameStateChange final
{
	GameState state{};
};
}//namespace network::commands
