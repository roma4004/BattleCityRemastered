#include "utils/Log.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace
{
//NOTE: guards the console and the file together - the network threads write from outside the game loop
std::mutex g_writeMutex;
}// namespace

bool Log::_consoleEnabled = true;
bool Log::_fileEnabled = false;
Log::Level Log::_level = Level::Normal;
std::string Log::_filename = "game_log.txt";

void Log::Error(const std::string& message) { Write(message, true); }

void Log::Info(const std::string& message)
{
	if (_level >= Level::Normal)
	{
		Write(message, false);
	}
}

void Log::Detail(const std::string& message)
{
	if (IsDetailed())
	{
		Write(message, false);
	}
}

//NOTE: read without the lock - the level is set once at startup, and locking here would serialise
//every frame's worth of skipped calls at the normal level
bool Log::IsDetailed() { return _level >= Level::Detailed; }

void Log::SetConsole(const bool enabled)
{
	const std::scoped_lock lock(g_writeMutex);
	_consoleEnabled = enabled;
}

void Log::SetLevel(const Level level)
{
	const std::scoped_lock lock(g_writeMutex);
	_level = level;
}

void Log::SetFile(const bool enabled, const std::string& filename)
{
	const std::scoped_lock lock(g_writeMutex);
	_fileEnabled = enabled;
	if (!filename.empty())
	{
		_filename = filename;
	}

	if (!enabled)
	{
		return;
	}

	//NOTE: truncate once here, append on every write - so a run starts clean without holding the
	//file open across the whole session
	if (std::ofstream file{_filename, std::ios::out | std::ios::trunc}; file.is_open())
	{
		file << TimeStamp() << " log started" << '\n';
	}
	else
	{
		_fileEnabled = false;
		std::cerr << "cannot open log file " << _filename << '\n';
	}
}

std::string Log::TimeStamp()
{
	const auto now = std::chrono::system_clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(now);
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::tm timeInfo{};
	localtime_s(&timeInfo, &nowTime);

	std::ostringstream stream;
	stream << '[' << std::put_time(&timeInfo, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count()
		   << ']';

	return stream.str();
}

void Log::Write(const std::string& message, const bool isError)
{
	const std::scoped_lock lock(g_writeMutex);
	const std::string line = TimeStamp() + ' ' + message;

	if (_consoleEnabled)
	{
		(isError ? std::cerr : std::cout) << line << '\n';
	}

	if (_fileEnabled)
	{
		if (std::ofstream file{_filename, std::ios::out | std::ios::app}; file.is_open())
		{
			file << (isError ? "ERROR " : "") << line << '\n';
		}
	}
}
