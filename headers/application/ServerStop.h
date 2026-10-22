#pragma once

#include <optional>
#include <stop_token>
#include <string>
#include <thread>

//NOTE: Ctrl+C, a closed console, /exit and the parent game's event as one stop flag; the loop then shuts down cleanly
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
	//NOTE: the game that started the server does not wait for it, so nothing else would end one that hangs
	void CutShortIfHung(const std::stop_token& stop) const;

	//NOTE: a HANDLE; Windows only
	[[maybe_unused]] void* _event{nullptr};
	std::jthread _watchdog{};
};
