#include "network/CommandDispatcher.h"
#include "network/commands/CommandBatch.h"
#include "network/Serializer.h"
#include "utils/Log.h"
#include <sstream>
#include <utility>

namespace network
{
CommandDispatcher::CommandDispatcher(std::string ownerName)
	: _ownerName(std::move(ownerName)) {}

void CommandDispatcher::RegisterAll(const std::initializer_list<std::pair<const CommandType, Handler>> handlers)
{
	_handlers.insert(handlers);
}

std::expected<void, DispatchError> CommandDispatcher::Dispatch(const std::string& archiveData)
{
	const auto batch = Deserialize(archiveData);
	if (!batch)
	{
		constexpr std::size_t maxLoggedBytes{200};
		const std::string rawData = archiveData.length() < maxLoggedBytes
										? archiveData
										: archiveData.substr(0, maxLoggedBytes) + "...";

		Log::Error(_ownerName + " deserialization: " + batch.error().reason + ", raw size "
				   + std::to_string(archiveData.length()) + ", raw data: " + rawData);

		return std::unexpected(DispatchError{.reason = batch.error().reason, .frameSize = archiveData.length()});
	}

	for (const auto& command: batch->commands)
	{
		if (const auto it = _handlers.find(commands::GetCommandType(command)); it != _handlers.end())
		{
			it->second(command);
		}
		else
		{
			Log::Error(_ownerName + " unhandled command type "
					   + std::to_string(static_cast<int>(commands::GetCommandType(command))));
		}
	}

	return {};
}
}//namespace network
