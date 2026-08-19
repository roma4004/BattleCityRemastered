#include "network/CommandDispatcher.h"
#include "network/commands/CommandBatch.h"
#include "utils/NetworkLogger.h"
#include <ser20/archives/portable_binary.hpp>
#include <sstream>
#include <utility>

namespace network
{
CommandDispatcher::CommandDispatcher(std::string ownerName)
	: _ownerName(std::move(ownerName))
{
}

void CommandDispatcher::RegisterAll(std::initializer_list<std::pair<const CommandType, Handler>> handlers)
{
	_handlers.insert(handlers);
}

std::expected<void, DispatchError> CommandDispatcher::Dispatch(const std::string& archiveData)
{
	commands::CommandBatch batch;
	try
	{
		std::istringstream archiveStream(archiveData);
		ser20::PortableBinaryInputArchive ia(archiveStream);
		ia(batch);
	}
	catch (const std::exception& e)
	{
		constexpr std::size_t maxLoggedBytes{200};
		const std::string rawData = archiveData.length() < maxLoggedBytes
											? archiveData
											: archiveData.substr(0, maxLoggedBytes) + "...";

		NetworkLogger::WriteError(_ownerName + " deserialization: " + e.what() + ", raw size "
								  + std::to_string(archiveData.length()) + ", raw data: " + rawData);

		return std::unexpected(DispatchError{.reason = e.what(), .frameSize = archiveData.length()});
	}

	for (const auto& command: batch.GetCommands())
	{
		if (const auto it = _handlers.find(commands::GetCommandType(command)); it != _handlers.end())
		{
			it->second(command);
		}
		else
		{
			NetworkLogger::WriteError(_ownerName + " unhandled command type "
									  + std::to_string(static_cast<int>(commands::GetCommandType(command))));
		}
	}

	return {};
}
}//namespace network
