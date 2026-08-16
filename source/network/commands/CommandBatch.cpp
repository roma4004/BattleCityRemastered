#include "network/commands/CommandBatch.h"
#include <numeric>

namespace network::commands
{
void CommandBatch::AddCommand(AnyCommand command) { _commands.push_back(std::move(command)); }

const std::vector<AnyCommand>& CommandBatch::GetCommands() const noexcept { return _commands; }

size_t CommandBatch::GetSize() const noexcept { return _commands.size(); }

bool CommandBatch::IsEmpty() const noexcept { return _commands.empty(); }

std::string CommandBatch::GetClassNamesW() const
{
	return std::accumulate(_commands.begin(), _commands.end(), std::string{},
						   [](std::string acc, const AnyCommand& command)
						   {
							   if (!acc.empty())
								   acc += ", ";
							   acc += GetClassNameW(command);
							   return acc;
						   });
}
}//namespace network::commands
