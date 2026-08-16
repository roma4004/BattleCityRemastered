#pragma once

#include "commands/AnyCommand.h"
#include "enums/CommandType.h"
#include <functional>
#include <initializer_list>
#include <utility>
#include <string>
#include <unordered_map>

namespace network
{
//NOTE: turns a received frame into calls on the registered handlers - the deserialise-and-route half
//that Client and Session used to carry each on its own
class CommandDispatcher final
{
public:
	using Handler = std::function<void(const commands::AnyCommand&)>;

	explicit CommandDispatcher(std::string ownerName);

	void RegisterAll(std::initializer_list<std::pair<const CommandType, Handler>> handlers);
	void Dispatch(const std::string& archiveData);

private:
	std::string _ownerName;
	std::unordered_map<CommandType, Handler> _handlers{};
};
}//namespace network
