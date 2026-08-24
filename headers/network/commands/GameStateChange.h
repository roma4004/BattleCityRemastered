#pragma once

#include "enums/CommandType.h"
#include "enums/GameState.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct GameStateChange final
{
	CommandType type{CommandType::GAME_STATE_CHANGE};
	GameState state{};
};
}//namespace network::commands
