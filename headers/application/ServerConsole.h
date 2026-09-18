#pragma once

#include <memory>
#include <string>
#include <vector>

//NOTE: the console is read on a thread of its own and handed over once a frame - what was typed is applied on
//the game thread, because EventSystem is not thread-safe
class ServerConsole final
{
public:
	ServerConsole();

	[[nodiscard]] std::vector<std::string> TakeLines();

private:
	struct Inbox;
	std::shared_ptr<Inbox> _inbox;
};
