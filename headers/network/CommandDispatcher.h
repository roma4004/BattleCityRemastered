#pragma once

#include "commands/AnyCommand.h"
#include "enums/CommandType.h"
#include <expected>
#include <functional>
#include <initializer_list>
#include <utility>
#include <string>
#include <unordered_map>

namespace network
{
//NOTE: no raw bytes - Dispatch dumps them itself, so callers need not repeat the formatting
struct DispatchError final
{
	std::string reason{};
	std::size_t frameSize{};
};

//NOTE: turns a received frame into calls on the registered handlers - the deserialise-and-route half
//that Client and Session used to carry each on its own
class CommandDispatcher final
{
public:
	using Handler = std::function<void(const commands::AnyCommand&)>;

	explicit CommandDispatcher(std::string ownerName);

	void RegisterAll(std::initializer_list<std::pair<const CommandType, Handler>> handlers);

	//NOTE: a command with no handler is not a failure - both peers share one AnyCommand, so each
	//side ignores the half addressed to the other. Only an unreadable frame is.
	[[nodiscard]] std::expected<void, DispatchError> Dispatch(const std::string& archiveData);

private:
	std::string _ownerName;
	std::unordered_map<CommandType, Handler> _handlers{};
};
}//namespace network
