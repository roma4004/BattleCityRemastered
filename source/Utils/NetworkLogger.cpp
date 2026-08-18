#include "utils/NetworkLogger.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

bool NetworkLogger::_fileLoggingEnabled = false;
bool NetworkLogger::_consoleLoggingEnabled = true;
int NetworkLogger::_verbosityLevel = 1;
std::string NetworkLogger::_logFilename = "network_log.txt";
std::mutex NetworkLogger::_logMutex;

void NetworkLogger::LogServerIn(const std::string& commandName)
{
	if (_verbosityLevel >= 1)
	{
		const std::string message = "[SERVER] received command: " + commandName;
		WriteLog(message);
	}
}

void NetworkLogger::LogServerOut(const std::string& commandName)
{
	if (_verbosityLevel >= 1)
	{
		const std::string message = "[SERVER] send command: " + commandName;
		WriteLog(message);
	}
}

void NetworkLogger::LogClientIn(const std::string& commandName)
{
	if (_verbosityLevel >= 1)
	{
		const std::string message = "[CLIENT] received command: " + commandName;
		WriteLog(message);
	}
}

void NetworkLogger::LogClientOut(const std::string& commandName)
{
	if (_verbosityLevel >= 1)
	{
		const std::string message = "[CLIENT] send command: " + commandName;
		WriteLog(message);
	}
}

void NetworkLogger::SetFileLogging(const bool enabled, const std::string& filename)
{
	std::scoped_lock lock(_logMutex);
	_fileLoggingEnabled = enabled;
	if (!filename.empty())
	{
		_logFilename = filename;
	}

	if (enabled)
	{
		// Створюємо новий файл або очищаємо існуючий при першому запуску
		std::ofstream logFile(_logFilename, std::ios::out | std::ios::trunc);
		if (logFile.is_open())
		{
			logFile << "=== start log " << GetCurrentTimeStamp() << " ===\n";
			logFile.close();
		}
	}
}

void NetworkLogger::SetConsoleLogging(const bool enabled)
{
	std::scoped_lock lock(_logMutex);
	_consoleLoggingEnabled = enabled;
}

void NetworkLogger::SetVerbosityLevel(const int level)
{
	std::scoped_lock lock(_logMutex);
	_verbosityLevel = level;
}

void NetworkLogger::WriteError(const std::string& message)
{
	std::scoped_lock lock(_logMutex);

	const std::string fullMessage = GetCurrentTimeStamp() + " ERROR " + message;

	if (_consoleLoggingEnabled)
	{
		std::cerr << fullMessage << '\n';
	}

	if (_fileLoggingEnabled)
	{
		std::ofstream logFile(_logFilename, std::ios::app);
		if (logFile.is_open())
		{
			logFile << fullMessage << '\n';
		}
	}
}

void NetworkLogger::WriteLog(const std::string& message, const bool skipEndl)
{
	std::scoped_lock lock(_logMutex);

	const std::string fullMessage = GetCurrentTimeStamp() + " " + message;

	// Вивід у консоль
	if (_consoleLoggingEnabled)
	{
		if (skipEndl)
		{
			std::cout << fullMessage;
		}
		else
		{
			std::cout << fullMessage << '\n';
		}
	}

	// Запис у файл
	if (_fileLoggingEnabled)
	{
		std::ofstream logFile(_logFilename, std::ios::app);
		if (logFile.is_open())
		{
			logFile << fullMessage << '\n';
			logFile.close();
		}
	}
}

std::string NetworkLogger::GetCurrentTimeStamp()
{
	const auto now = std::chrono::system_clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(now);
	const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::tm timeStruct;
	localtime_s(&timeStruct, &nowTime);

	std::stringstream ss;
	ss << '[' << std::put_time(&timeStruct, "%H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << nowMs.count()
			<< ']';

	return ss.str();
}
