#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>

class Logger
{
public:
	static Logger& GetInstance();

	void Log(const std::string& message);
	void LogTankSpawn(const std::string& tankName, const std::string& fraction, const std::string& uuid);
	static std::string GetCurrentTimeString();

private:
	Logger();
	~Logger();

	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	std::ofstream _logFile;
};
