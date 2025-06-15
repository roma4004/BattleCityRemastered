#include "../../headers/utils/Logger.h"
#include <iostream>
#include <sstream>

Logger::Logger()
{
	_logFile.open("game_log.txt", std::ios::out | std::ios::app);
	if (!_logFile.is_open())
	{
		std::cerr << "Failed to open log file!" << std::endl;
	}

	Log("Logger initialized");
}

Logger::~Logger()
{
	if (_logFile.is_open())
	{
		Log("Logger shutdown");
		_logFile.close();
	}
}

Logger& Logger::GetInstance()
{
	static Logger instance;
	return instance;
}

void Logger::Log(const std::string& message)
{
	if (_logFile.is_open())
	{
		_logFile << GetCurrentTimeString() << " - " << message << std::endl;
		_logFile.flush();
	}
}

void Logger::LogTankSpawn(const std::string& tankName, const std::string& fraction, const std::string& uuid)
{
	std::stringstream ss;
	ss << "Spawned tank: " << tankName << ", Fraction: " << fraction << ", UUID: " << uuid;
	Log(ss.str());
}

std::string Logger::GetCurrentTimeString()
{
	const auto now = std::chrono::system_clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(now);
	const auto ms = std::chrono::duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

	std::tm timeInfo;
	localtime_s(&timeInfo, &nowTime);

	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%H:%M:%S") << '.'
			<< std::setfill('0') << std::setw(3) << ms.count();

	return ss.str();
}
