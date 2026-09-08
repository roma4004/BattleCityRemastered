#include "application/ServerProcess.h"
#include "utils/Log.h"
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <windows.h>

namespace
{
constexpr auto kServerExeName{L"BattleCityServer.exe"};

[[nodiscard]] std::string LastErrorText(const char* what)
{
	return std::string(what) + " failed with " + std::to_string(GetLastError());
}

//NOTE: our own directory, not the working one - a debugger starts the game from anywhere
[[nodiscard]] std::filesystem::path ServerExePath()
{
	std::wstring self(MAX_PATH, L'\0');
	const DWORD written = GetModuleFileNameW(nullptr, self.data(), static_cast<DWORD>(self.size()));
	if (written == 0u || written == self.size())
	{
		return {};
	}

	self.resize(written);

	return std::filesystem::path(self).replace_filename(kServerExeName);
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

bool ServerProcess::Start()
{
	if (IsRunning())
	{
		return true;
	}

	const std::filesystem::path exe = ServerExePath();
	std::error_code ec;
	if (exe.empty() || !std::filesystem::exists(exe, ec))
	{
		Log::Error("ServerProcess: " + exe.string() + " is not next to the game exe");

		return false;
	}

	auto process = std::make_unique<Process>();

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
	std::wstring commandLine = L"\"" + exe.wstring() + L"\"";

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
