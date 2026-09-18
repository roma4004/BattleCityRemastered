#pragma once

#include "enums/GameState.h"

namespace network::commands
{
struct GameStateChange final
{
	GameState state{};
};
}//namespace network::commands
