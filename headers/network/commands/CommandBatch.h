#pragma once

#include "AnyCommand.h"
#include <vector>

namespace network::commands
{
struct CommandBatch
{
	std::vector<AnyCommand> commands;
};
}//namespace network::commands
