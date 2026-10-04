#pragma once

#include <optional>
#include <string>

//NOTE: every way the server is told to stop - Ctrl+C, a closed console window, /exit, and the game that started
//it - read by the loop as one answer. The loop then winds down on its own: the players are told goodbye and the
//port opened on the router is closed, whichever way the stop came
class ServerStop final
{
public:
	//NOTE: the event the game that started the server sets, by name - none for a server started by hand
	explicit ServerStop(const std::optional<std::string>& eventName);

	//NOTE: the last thing the server does - a closed console window holds the process open until this ran
	~ServerStop();

	ServerStop(const ServerStop&) = delete;
	ServerStop& operator=(const ServerStop&) = delete;
	ServerStop(ServerStop&&) = delete;
	ServerStop& operator=(ServerStop&&) = delete;

	void Request() noexcept;

	[[nodiscard]] bool IsRequested() const;

private:
	//NOTE: a HANDLE, held as what it is underneath so windows.h stays out of this header; Windows only
	[[maybe_unused]] void* _event{nullptr};
};
