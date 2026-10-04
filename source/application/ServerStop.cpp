#include "application/ServerStop.h"
#include "utils/Log.h"
#include <atomic>
#include <csignal>
#include <optional>
#include <string>
#ifdef _WIN32
#include <chrono>
#include <thread>
#include <windows.h>
#endif

namespace
{
std::atomic_bool isRequested{};
std::atomic_bool isWoundDown{};

extern "C" void OnStopSignal(int) { isRequested.store(true, std::memory_order_relaxed); }

#ifdef _WIN32
using namespace std::chrono_literals;

//NOTE: Windows gives a closing console five seconds, then ends the process - the wind-down gets a little less
constexpr auto kClosePatience{4500ms};
constexpr auto kClosePollStep{10ms};

//NOTE: a closed console window raises no signal - the CRT turns only Ctrl+C and Ctrl+Break into one - and the
//process is ended the moment this returns, so it is held here until the loop has wound down
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
					   + "), the game that started it can only kill it");
		}
	}
#else
	if (eventName)
	{
		Log::Error("server: --stop-event is a Windows one, ignored");
	}
#endif
}

ServerStop::~ServerStop()
{
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
