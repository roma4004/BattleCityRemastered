#include "network/commands/CommandBatch.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(CommandBatch)

CommandBatch::CommandBatch() : Command(CommandType::COMMAND_BATCH)
{
	//_commands.reserve(1000);
}

void CommandBatch::AddCommand(const std::shared_ptr<Command>& command)
{
	if (command)
	{
		_commands.push_back(command);
	}
}

const std::vector<std::shared_ptr<Command>>& CommandBatch::GetCommands() const noexcept { return _commands; }

const char* CommandBatch::GetClassNameW() const noexcept { return "CommandBatch"; }

size_t CommandBatch::GetSize() const noexcept { return _commands.size(); }

bool CommandBatch::IsEmpty() const noexcept { return _commands.empty(); }
