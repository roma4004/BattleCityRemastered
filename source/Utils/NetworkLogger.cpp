#include "utils/NetworkLogger.h"
#include <chrono>
#include <fstream>
#include <iostream>

bool NetworkLogger::_fileLoggingEnabled = false;
bool NetworkLogger::_consoleLoggingEnabled = true;
int NetworkLogger::_verbosityLevel = 1;
std::string NetworkLogger::_logFilename = "network_log.txt";
std::mutex NetworkLogger::_logMutex;

void NetworkLogger::LogServerReceive(const std::string& commandName)
{
	if (_verbosityLevel >= 1)
	{
		const std::string message = "[SERVER] received command: " + commandName;
		WriteLog(message);
	}
}

void NetworkLogger::LogServerSend(const std::string& commandName)
{
	if (_verbosityLevel >= 1)
	{
		const std::string message = "[SERVER] send command: " + commandName;
		WriteLog(message);
	}
}

void NetworkLogger::LogClientReceive(const std::string& commandName)
{
	if (_verbosityLevel >= 1)
	{
		const std::string message = "[CLIENT] received command: " + commandName;
		WriteLog(message);
	}
}

void NetworkLogger::LogClientSend(const std::string& commandName)
{
	if (_verbosityLevel >= 1)
	{
		const std::string message = "[CLIENT] send command: " + commandName;
		WriteLog(message);
	}
}

void NetworkLogger::SetFileLogging(bool enabled, const std::string& filename)
{
	std::lock_guard<std::mutex> lock(_logMutex);
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

void NetworkLogger::SetConsoleLogging(bool enabled)
{
	std::lock_guard<std::mutex> lock(_logMutex);
	_consoleLoggingEnabled = enabled;
}

void NetworkLogger::SetVerbosityLevel(int level)
{
	std::lock_guard<std::mutex> lock(_logMutex);
	_verbosityLevel = level;
}

void NetworkLogger::WriteLog(const std::string& message, const bool skipEndl)
{
	std::lock_guard<std::mutex> lock(_logMutex);

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
			std::cout << fullMessage << std::endl;
		}
	}

	// Запис у файл
	if (_fileLoggingEnabled)
	{
		std::ofstream logFile(_logFilename, std::ios::app);
		if (logFile.is_open())
		{
			logFile << fullMessage << std::endl;
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
	ss << '[' << std::put_time(&timeStruct, "%H:%M:%S") << '.'
			<< std::setfill('0') << std::setw(3) << nowMs.count() << ']';

	return ss.str();
}
