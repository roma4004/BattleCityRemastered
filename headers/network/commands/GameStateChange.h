#pragma once

#include "enums/CommandType.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct GameStateChange final
{
	CommandType type{CommandType::GAME_STATE_CHANGE};
	std::string gameState{};
};
}//namespace network::commands
