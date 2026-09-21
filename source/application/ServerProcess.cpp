#include "application/ServerProcess.h"
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
#include <thread>
#include <windows.h>

namespace
{
constexpr auto kServerExeName{L"BattleCityServer.exe"};
//NOTE: relative, so the name on the command line stays ASCII whatever the install path is - the child
//runs with the exe's folder as its working directory. Two games hosting at once would share it, and the
//second one would read the first one's port - a lobby is what fixes that, not a longer name
constexpr auto kPortFileName{"server-port.txt"};
constexpr auto kPortWait{std::chrono::seconds{5}};
constexpr auto kPortPollStep{std::chrono::milliseconds{20}};

//NOTE: the three options the child reads back - the host stays a bare literal, the port is its own word
std::string ChildArguments(const network::ServerAddress& address)
{
	return " --address=" + address.host + " --port=" + std::to_string(address.port) + " --port-file="
		   + kPortFileName;
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

//NOTE: the game dials the port, so it has to be in hand before the client node is built - hence a wait
//here rather than a retry around the first connect. A child that died says so at once, without the timeout
std::optional<std::uint16_t> WaitForPort(const std::filesystem::path& portFile, const HANDLE process)
{
	for (auto waited{std::chrono::milliseconds::zero()}; waited < kPortWait; waited += kPortPollStep)
	{
		if (const std::optional<std::uint16_t> port{ReadPortFile(portFile)})
		{
			return port;
		}

		if (WaitForSingleObject(process, 0u) != WAIT_TIMEOUT)
		{
			return std::nullopt;
		}

		std::this_thread::sleep_for(kPortPollStep);
	}

	return std::nullopt;
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

	~Process()
	{
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
	}
};

ServerProcess::ServerProcess() = default;

ServerProcess::~ServerProcess() = default;

std::optional<std::uint16_t> ServerProcess::Start(const network::ServerAddress& address)
{
	if (IsRunning())
	{
		return _boundPort;
	}

	const std::filesystem::path exe{ServerExePath()};
	std::error_code ec;
	if (exe.empty() || !std::filesystem::exists(exe, ec))
	{
		Log::Error("ServerProcess: " + exe.string() + " is not next to the game exe");

		return std::nullopt;
	}

	//NOTE: gone before the child starts, so what turns up later is this run's port and not the last one's
	const std::filesystem::path portFile{PortFilePath()};
	std::filesystem::remove(portFile, ec);

	auto process{std::make_unique<Process>()};

	process->job = CreateJobObjectW(nullptr, nullptr);
	if (process->job == nullptr)
	{
		Log::Error("ServerProcess: " + LastErrorText("CreateJobObject"));

		return std::nullopt;
	}

	//NOTE: the OS closes the handle for us if the game crashes without running its destructor
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
	limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (SetInformationJobObject(process->job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) == 0)
	{
		Log::Error("ServerProcess: " + LastErrorText("SetInformationJobObject"));

		return std::nullopt;
	}

	//NOTE: CreateProcess writes into this buffer, so it cannot be a literal
	const std::string argument{ChildArguments(address)};
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

		return std::nullopt;
	}

	process->process = info.hProcess;
	process->thread = info.hThread;

	if (AssignProcessToJobObject(process->job, info.hProcess) == 0)
	{
		Log::Error("ServerProcess: " + LastErrorText("AssignProcessToJobObject"));
		TerminateProcess(info.hProcess, 1u);

		return std::nullopt;
	}

	ResumeThread(info.hThread);

	_process = std::move(process);

	_boundPort = WaitForPort(portFile, info.hProcess);
	if (!_boundPort)
	{
		Log::Error("ServerProcess: BattleCityServer never reported a port");
		_process.reset();

		return std::nullopt;
	}

	Log::Info("ServerProcess: started BattleCityServer on port " + std::to_string(*_boundPort));

	return _boundPort;
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
