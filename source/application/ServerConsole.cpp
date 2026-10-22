#include "application/ServerConsole.h"
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

struct ServerConsole::Inbox final
{
	std::mutex mutex;
	std::vector<std::string> lines;
};

//NOTE: detached - a thread blocked in getline cannot be joined; it shares the inbox and ends with the process
ServerConsole::ServerConsole()
	: _inbox{std::make_shared<Inbox>()}
{
	std::thread{[inbox = _inbox]
	{
		for (std::string line; std::getline(std::cin, line);)
		{
			const std::scoped_lock lock{inbox->mutex};
			inbox->lines.push_back(std::move(line));
		}
	}}.detach();
}

std::vector<std::string> ServerConsole::TakeLines()
{
	const std::scoped_lock lock{_inbox->mutex};

	return std::exchange(_inbox->lines, {});
}
