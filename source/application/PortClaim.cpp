#include "application/PortClaim.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <chrono>
#include <cstdint>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <thread>
#include <unistd.h>
#endif

namespace
{
using namespace std::chrono_literals;

//NOTE: longer than the old server's own wind-down limit - it is surely gone by then
constexpr auto kPatience{12s};
}//namespace

PortClaim::PortClaim(const std::uint16_t port) { Take(port); }

#ifdef _WIN32
PortClaim::~PortClaim()
{
	if (_isHeld)
	{
		ReleaseMutex(_mutex);
	}

	if (_mutex != nullptr)
	{
		CloseHandle(_mutex);
	}
}

void PortClaim::Take(const std::uint16_t port)
{
	if (_mutex != nullptr || port == network::kAnyFreePort)
	{
		return;
	}

	const std::string name{"Local\\BattleCityServer.port." + std::to_string(port)};
	_mutex = CreateMutexA(nullptr, FALSE, name.c_str());
	if (_mutex == nullptr)
	{
		Log::Error("server: no claim on port " + std::to_string(port) + " (" + std::to_string(GetLastError())
				   + "), a server still on it is not waited for");

		return;
	}

	//NOTE: abandoned is held too - the server before ended without letting go
	DWORD claimed{WaitForSingleObject(_mutex, 0u)};
	if (claimed == WAIT_TIMEOUT)
	{
		Log::Info("server: waiting for the server before on port " + std::to_string(port) + " to wind down");
		claimed = WaitForSingleObject(_mutex, static_cast<DWORD>(std::chrono::milliseconds{kPatience}.count()));
	}

	_isHeld = claimed == WAIT_OBJECT_0 || claimed == WAIT_ABANDONED;
}
#else
PortClaim::~PortClaim()
{
	if (_lockFile != -1)
	{
		close(_lockFile);
	}
}

void PortClaim::Take(const std::uint16_t port)
{
	if (_lockFile != -1 || port == network::kAnyFreePort)
	{
		return;
	}

	const std::string path{"/tmp/BattleCityServer.port." + std::to_string(port)};
	//NOTE: read-only is enough for flock, and opens the file another user's server left behind as well
	_lockFile = open(path.c_str(), O_RDONLY | O_CREAT | O_CLOEXEC, 0644);
	if (_lockFile == -1)
	{
		Log::Error("server: no claim on port " + std::to_string(port) + " (" + std::to_string(errno)
				   + "), a server still on it is not waited for");

		return;
	}

	if (flock(_lockFile, LOCK_EX | LOCK_NB) == 0)
	{
		return;
	}

	Log::Info("server: waiting for the server before on port " + std::to_string(port) + " to wind down");
	//NOTE: flock has no timeout of its own, so it is asked again until the patience runs out
	const auto deadline{std::chrono::steady_clock::now() + kPatience};
	while (flock(_lockFile, LOCK_EX | LOCK_NB) != 0 && std::chrono::steady_clock::now() < deadline)
	{
		std::this_thread::sleep_for(100ms);
	}
}
#endif
