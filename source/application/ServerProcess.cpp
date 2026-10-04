#include "application/ServerProcess.h"
#include "components/MatchSettings.h"
#include "enums/MatchRules.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <windows.h>

using namespace std::chrono_literals;

namespace
{
constexpr auto kServerExeName{L"BattleCityServer.exe"};
//NOTE: a server winding down tells its players goodbye and closes its port on the router - a router that does not
//answer is not waited for longer than this, and the job kills the server then
constexpr auto kStopPatience{3s};
//NOTE: relative, so the name on the command line stays ASCII whatever the install path is - the child
//runs with the exe's folder as its working directory. Two games hosting at once would share it, and the
//second one would read the first one's port - a lobby is what fixes that, not a longer name
constexpr auto kPortFileName{"server-port.txt"};

//NOTE: unique to this game and this start - two games hosting at once, or one hosting again, never share one
[[nodiscard]] std::string StopEventName()
{
	static unsigned int starts{};

	return "Local\\BattleCityServer.stop." + std::to_string(GetCurrentProcessId()) + '.' + std::to_string(++starts);
}

//NOTE: what the child reads back - the host stays a bare literal, the port is its own word, the map a bare name
std::string ChildArguments(const network::ServerAddress& address, const MatchSettings& match,
						   const bool isPortForwarded, const std::string& stopEventName)
{
	const std::string rules{match.rules == MatchRules::FreeForAll ? "ffa" : "classic"};

	return " --address=" + address.host + " --port=" + std::to_string(address.port) + " --port-file="
		   + kPortFileName + " --seats=" + std::to_string(match.seats) + " --rules=" + rules + " --map=" + match.map
		   + " --enemies=" + std::to_string(match.enemiesAtOnce) + " --bots=" + std::to_string(match.bots)
		   + " --start=" + (match.isStartingAtOnce ? "now" : "full") + (isPortForwarded ? " --upnp" : "")
		   + " --stop-event=" + stopEventName;
}

std::optional<std::uint16_t> ReadPortFile(const std::filesystem::path& path)
{
	std::ifstream file{path};
	std::string text;
	if (!(file >> text))
	{
		return std::nullopt;
	}

	unsigned port{};
	const auto* const first{text.data()};
	const auto* const last{first + text.size()};
	const auto [ptr, error] = std::from_chars(first, last, port);
	if (error != std::errc{} || ptr != last || port == 0u || port > 65535u)
	{
		return std::nullopt;
	}

	return static_cast<std::uint16_t>(port);
}

[[nodiscard]] std::string LastErrorText(const char* what)
{
	return std::string(what) + " failed with " + std::to_string(GetLastError());
}

//NOTE: our own directory, not the working one - a debugger starts the game from anywhere
[[nodiscard]] std::filesystem::path ServerExePath()
{
	std::wstring self(MAX_PATH, L'\0');
	const DWORD written{GetModuleFileNameW(nullptr, self.data(), static_cast<DWORD>(self.size()))};
	if (written == 0u || written == self.size())
	{
		return {};
	}

	self.resize(written);

	return std::filesystem::path(self).replace_filename(kServerExeName);
}

//NOTE: next to our own exe, never the working directory - the child is started there and writes the
//name relative to it, and a debugger starts the game from wherever it likes
[[nodiscard]] std::filesystem::path PortFilePath()
{
	const std::filesystem::path exe{ServerExePath()};

	return exe.empty() ? std::filesystem::path{} : exe.parent_path() / kPortFileName;
}
}//namespace

struct ServerProcess::Process
{
	HANDLE job{nullptr};
	HANDLE process{nullptr};
	HANDLE thread{nullptr};
	//NOTE: set to have the child wind down by itself
	HANDLE stopEvent{nullptr};

	~Process()
	{
		Stop();

		//NOTE: the job first - closing it is what kills the child; the other two only observe it
		if (job != nullptr)
		{
			CloseHandle(job);
		}

		if (thread != nullptr)
		{
			CloseHandle(thread);
		}

		if (process != nullptr)
		{
			CloseHandle(process);
		}

		if (stopEvent != nullptr)
		{
			CloseHandle(stopEvent);
		}
	}

	//NOTE: asked first and killed only past the patience - a killed server leaves its port open on the router
	void Stop() const
	{
		if (process == nullptr || stopEvent == nullptr || SetEvent(stopEvent) == 0)
		{
			return;
		}

		const auto patience{static_cast<DWORD>(std::chrono::milliseconds{kStopPatience}.count())};
		if (WaitForSingleObject(process, patience) == WAIT_TIMEOUT)
		{
			Log::Error("ServerProcess: BattleCityServer did not stop in time and is killed");
		}
	}
};

ServerProcess::ServerProcess() = default;

ServerProcess::~ServerProcess() = default;

bool ServerProcess::Start(const network::ServerAddress& address, const MatchSettings& match,
						  const bool isPortForwarded)
{
	if (IsRunning())
	{
		return true;
	}

	const std::filesystem::path exe{ServerExePath()};
	std::error_code ec;
	if (exe.empty() || !std::filesystem::exists(exe, ec))
	{
		Log::Error("ServerProcess: " + exe.string() + " is not next to the game exe");

		return false;
	}

	//NOTE: gone before the child starts, so what turns up later is this run's port and not the last one's
	const std::filesystem::path portFile{PortFilePath()};
	std::filesystem::remove(portFile, ec);

	auto process{std::make_unique<Process>()};

	const std::string stopEventName{StopEventName()};
	process->stopEvent = CreateEventA(nullptr, TRUE, FALSE, stopEventName.c_str());
	if (process->stopEvent == nullptr)
	{
		Log::Error("ServerProcess: " + LastErrorText("CreateEvent"));

		return false;
	}

	process->job = CreateJobObjectW(nullptr, nullptr);
	if (process->job == nullptr)
	{
		Log::Error("ServerProcess: " + LastErrorText("CreateJobObject"));

		return false;
	}

	//NOTE: the OS closes the handle for us if the game crashes without running its destructor
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
	limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (SetInformationJobObject(process->job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) == 0)
	{
		Log::Error("ServerProcess: " + LastErrorText("SetInformationJobObject"));

		return false;
	}

	//NOTE: CreateProcess writes into this buffer, so it cannot be a literal
	const std::string argument{ChildArguments(address, match, isPortForwarded, stopEventName)};
	std::wstring commandLine{L"\"" + exe.wstring() + L"\"" + std::wstring(argument.begin(), argument.end())};

	STARTUPINFOW startup{};
	startup.cb = sizeof(startup);
	//NOTE: a new console takes the foreground by default, and it is started while the game window is
	//already up - so it would land on top of it. Nothing is lost: the same log goes to the file
	startup.dwFlags = STARTF_USESHOWWINDOW;
	startup.wShowWindow = SW_SHOWMINNOACTIVE;
	PROCESS_INFORMATION info{};

	//NOTE: suspended until it is in the job - a grandchild spawned before that would outlive the game
	if (CreateProcessW(exe.c_str(), commandLine.data(), nullptr, nullptr, FALSE,
					   CREATE_NEW_CONSOLE | CREATE_SUSPENDED, nullptr, exe.parent_path().wstring().c_str(),
					   &startup, &info) == 0)
	{
		Log::Error("ServerProcess: " + LastErrorText("CreateProcess"));

		return false;
	}

	process->process = info.hProcess;
	process->thread = info.hThread;

	if (AssignProcessToJobObject(process->job, info.hProcess) == 0)
	{
		Log::Error("ServerProcess: " + LastErrorText("AssignProcessToJobObject"));
		TerminateProcess(info.hProcess, 1u);

		return false;
	}

	ResumeThread(info.hThread);

	_process = std::move(process);
	Log::Info("ServerProcess: started BattleCityServer");

	return true;
}

std::optional<std::uint16_t> ServerProcess::PublishedPort()
{
	return ReadPortFile(PortFilePath());
}

bool ServerProcess::IsRunning() const
{
	if (!_process || _process->process == nullptr)
	{
		return false;
	}

	return WaitForSingleObject(_process->process, 0u) == WAIT_TIMEOUT;
}
