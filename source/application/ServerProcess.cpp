#include "application/ServerProcess.h"
#include "application/CommandLineParser.h"
#include "components/MatchSettings.h"
#include "enums/MatchRules.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#endif

namespace
{
#ifdef _WIN32
constexpr auto kServerExeName{L"BattleCityServer.exe"};
#else
constexpr auto kServerExeName{"BattleCityServer"};
#endif
//NOTE: relative, so the name on the command line stays ASCII whatever the install path is - the child
//runs with the exe's folder as its working directory. Two games hosting at once would share it, and the
//second one would read the first one's port - a lobby is what fixes that, not a longer name
constexpr auto kPortFileName{"server-port.txt"};

//NOTE: what the child reads back - the host stays a bare literal, the port is its own word, the map a bare name
std::vector<std::string> ChildArguments(const network::ServerAddress& address, const MatchSettings& match,
										const bool isPortForwarded, const std::string& routerUrl)
{
	const std::string rules{match.rules == MatchRules::FreeForAll ? "ffa" : "classic"};
	std::vector<std::string> arguments{"--address=" + address.host,
									   "--port=" + std::to_string(address.port),
									   std::string{"--port-file="} + kPortFileName,
									   "--seats=" + std::to_string(match.seats),
									   "--rules=" + rules,
									   "--map=" + match.map,
									   "--enemies=" + std::to_string(match.enemiesAtOnce),
									   "--bots=" + std::to_string(match.bots),
									   std::string{"--start="} + (match.isStartingAtOnce ? "now" : "full")};
	if (isPortForwarded)
	{
		arguments.push_back(routerUrl.empty() ? "--upnp" : "--upnp=" + routerUrl);
	}

	return arguments;
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

#ifdef _WIN32
//NOTE: unique to this game and this start - two games hosting at once, or one hosting again, never share one
[[nodiscard]] std::string StopEventName()
{
	static unsigned int starts{};

	return "Local\\BattleCityServer.stop." + std::to_string(GetCurrentProcessId()) + '.' + std::to_string(++starts);
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
#else
[[nodiscard]] std::string ErrnoText(const char* what)
{
	return std::string(what) + " failed: " + std::system_category().message(errno);
}

//NOTE: our own directory, not the working one - a debugger starts the game from anywhere
[[nodiscard]] std::filesystem::path ServerExePath()
{
	std::error_code ec;
	const std::filesystem::path self{std::filesystem::read_symlink("/proc/self/exe", ec)};

	return ec ? std::filesystem::path{} : self.parent_path() / kServerExeName;
}
#endif

//NOTE: next to our own exe, never the working directory - the child is started there and writes the
//name relative to it, and a debugger starts the game from wherever it likes
[[nodiscard]] std::filesystem::path PortFilePath()
{
	const std::filesystem::path exe{ServerExePath()};

	return exe.empty() ? std::filesystem::path{} : exe.parent_path() / kPortFileName;
}
}//namespace

#ifdef _WIN32
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

		//NOTE: the job first - closing it kills a child that was not let go; the other two only observe it
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

	//NOTE: asked to wind down and let out of the job - the game does not wait for it; killed when it cannot be asked
	void Stop() const
	{
		if (process == nullptr || stopEvent == nullptr || SetEvent(stopEvent) == 0)
		{
			return;
		}

		JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
		if (SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) == 0)
		{
			Log::Error("ServerProcess: " + LastErrorText("SetInformationJobObject") + ", the server is killed");
		}
	}
};

ServerProcess::ServerProcess() = default;

ServerProcess::~ServerProcess() = default;

bool ServerProcess::Start(const network::ServerAddress& address, const MatchSettings& match,
						  const bool isPortForwarded, const std::string& routerUrl)
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
	std::string argument{};
	for (const std::string& word: ChildArguments(address, match, isPortForwarded, routerUrl))
	{
		argument += ' ' + word;
	}

	argument += " --stop-event=" + stopEventName;
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

bool ServerProcess::IsRunning() const
{
	if (!_process || _process->process == nullptr)
	{
		return false;
	}

	return WaitForSingleObject(_process->process, 0u) == WAIT_TIMEOUT;
}

bool ServerProcess::CouldNotListen() const
{
	DWORD code{};

	return _process && _process->process != nullptr && GetExitCodeProcess(_process->process, &code) != 0
		   && code == static_cast<DWORD>(kServerExitCannotListen);
}
#else
struct ServerProcess::Process
{
	pid_t pid{};
	//NOTE: what it exited with, once reaped - a child is reaped once and asked about more often than that
	std::optional<int> status{};

	~Process() { Stop(); }

	//NOTE: asked, not waited for - the server winds down by itself. Reaped off the game thread, or it stays a zombie
	void Stop() const
	{
		if (status || kill(pid, SIGTERM) != 0)
		{
			return;
		}

		std::thread{[child = pid] { waitpid(child, nullptr, 0); }}.detach();
	}

	[[nodiscard]] bool IsRunning()
	{
		if (status)
		{
			return false;
		}

		int code{};
		if (waitpid(pid, &code, WNOHANG) != pid)
		{
			return true;
		}

		status = code;

		return false;
	}
};

ServerProcess::ServerProcess() = default;

ServerProcess::~ServerProcess() = default;

bool ServerProcess::Start(const network::ServerAddress& address, const MatchSettings& match,
						  const bool isPortForwarded, const std::string& routerUrl)
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

	//NOTE: built before the fork - between it and exec the child may only make calls that allocate nothing
	const std::string program{exe.string()};
	const std::string directory{exe.parent_path().string()};
	std::vector<std::string> words{ChildArguments(address, match, isPortForwarded, routerUrl)};
	words.insert(words.begin(), program);
	std::vector<char*> argv{};
	for (std::string& word: words)
	{
		argv.push_back(word.data());
	}

	argv.push_back(nullptr);

	const pid_t parent{getpid()};
	const pid_t pid{fork()};
	if (pid < 0)
	{
		Log::Error("ServerProcess: " + ErrnoText("fork"));

		return false;
	}

	if (pid == 0)
	{
		//NOTE: the game gone, killed or crashed, the server is told as the game would tell it - it winds down and
		//closes its port. Tied to the thread that forked, and that is the game's main one, which lives to the end
		prctl(PR_SET_PDEATHSIG, SIGTERM);
		if (getppid() != parent)
		{
			_exit(EXIT_FAILURE);
		}

		//NOTE: the game's own terminal input is not the server's - its console would wait on it to the end
		if (const int nothing{open("/dev/null", O_RDONLY)}; nothing >= 0)
		{
			dup2(nothing, STDIN_FILENO);
			close(nothing);
		}

		if (chdir(directory.c_str()) == 0)
		{
			execv(program.c_str(), argv.data());
		}

		_exit(EXIT_FAILURE);
	}

	_process = std::make_unique<Process>(Process{.pid = pid});
	Log::Info("ServerProcess: started BattleCityServer");

	return true;
}

bool ServerProcess::IsRunning() const { return _process && _process->IsRunning(); }

bool ServerProcess::CouldNotListen() const
{
	if (!_process || _process->IsRunning())
	{
		return false;
	}

	const std::optional<int>& status{_process->status};

	return status && WIFEXITED(*status) && WEXITSTATUS(*status) == kServerExitCannotListen;
}
#endif

std::optional<std::uint16_t> ServerProcess::PublishedPort()
{
	return ReadPortFile(PortFilePath());
}
