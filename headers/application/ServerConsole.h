#pragma once

#include <memory>
#include <string>
#include <vector>

//NOTE: stdin is read on its own thread; lines are applied on the game thread since EventSystem is not thread-safe
class ServerConsole final
{
public:
	ServerConsole();

	[[nodiscard]] std::vector<std::string> TakeLines();

private:
	struct Inbox;
	std::shared_ptr<Inbox> _inbox;
};
