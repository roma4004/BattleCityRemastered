#include "application/ServerStop.h"
#include "utils/Log.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std::chrono_literals;

namespace
{
//NOTE: plenty for the goodbye and the port on the router - past it a router that does not answer is not waited for
constexpr auto kWindDownPatience{10s};
constexpr auto kWatchStep{50ms};

std::atomic_bool isRequested{};
std::atomic_bool isWoundDown{};

extern "C" void OnStopSignal(int) { isRequested.store(true, std::memory_order_relaxed); }

#ifdef _WIN32
//NOTE: Windows gives a closing console five seconds, then ends the process - the wind-down gets a little less
constexpr auto kClosePatience{4500ms};
constexpr auto kClosePollStep{10ms};

//NOTE: a closed console raises no signal, and the process ends once this returns - so it waits for the wind-down
BOOL WINAPI OnConsoleClosed(const DWORD event)
{
	if (event != CTRL_CLOSE_EVENT && event != CTRL_LOGOFF_EVENT && event != CTRL_SHUTDOWN_EVENT)
	{
		return FALSE;
	}

	isRequested.store(true, std::memory_order_relaxed);
	const auto deadline{std::chrono::steady_clock::now() + kClosePatience};
	while (!isWoundDown.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline)
	{
		std::this_thread::sleep_for(kClosePollStep);
	}

	return TRUE;
}
#endif
}//namespace

ServerStop::ServerStop(const std::optional<std::string>& eventName)
{
	std::signal(SIGINT, OnStopSignal);
	std::signal(SIGTERM, OnStopSignal);
#ifdef SIGHUP
	//NOTE: a closed terminal - the Linux side of a closed console window
	std::signal(SIGHUP, OnStopSignal);
#endif

#ifdef _WIN32
	SetConsoleCtrlHandler(OnConsoleClosed, TRUE);
	if (eventName)
	{
		_event = OpenEventA(SYNCHRONIZE, FALSE, eventName->c_str());
		if (_event == nullptr)
		{
			Log::Error("server: no stop event " + *eventName + " (" + std::to_string(GetLastError())
					   + "), and the game that started it could not stop it - stopping at once");
			Request();
		}
	}
#else
	if (eventName)
	{
		Log::Error("server: --stop-event is a Windows one, ignored");
	}
#endif

	_watchdog = std::jthread{[this](const std::stop_token& stop) { CutShortIfHung(stop); }};
}

ServerStop::~ServerStop()
{
	//NOTE: first, while the event it reads is still open
	_watchdog.request_stop();
	_watchdog.join();

#ifdef _WIN32
	if (_event != nullptr)
	{
		CloseHandle(_event);
	}
#endif

	isWoundDown.store(true, std::memory_order_release);
}

void ServerStop::Request() noexcept { isRequested.store(true, std::memory_order_relaxed); }

bool ServerStop::IsRequested() const
{
	if (isRequested.load(std::memory_order_relaxed))
	{
		return true;
	}

#ifdef _WIN32
	return _event != nullptr && WaitForSingleObject(_event, 0u) == WAIT_OBJECT_0;
#else
	return false;
#endif
}

void ServerStop::CutShortIfHung(const std::stop_token& stop) const
{
	std::optional<std::chrono::steady_clock::time_point> deadline{};
	while (!stop.stop_requested())
	{
		const auto now{std::chrono::steady_clock::now()};
		if (!deadline && IsRequested())
		{
			deadline = now + kWindDownPatience;
		}

		if (deadline && now >= *deadline)
		{
			Log::Error("server: not wound down in " + std::to_string(std::chrono::seconds{kWindDownPatience}.count())
					   + " s, ended outright");
			std::_Exit(EXIT_FAILURE);
		}

		std::this_thread::sleep_for(kWatchStep);
	}
}
