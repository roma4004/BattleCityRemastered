#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

/**
 * @brief central network logs
 */
class NetworkLogger
{
public:
    /**
     * @brief Logging server receives
     * @param commandName Назва команди
     */
    static void LogServerReceive(const std::string& commandName);

    /**
     * @brief Logging server sends
     * @param commandName Назва команди
     */
    static void LogServerSend(const std::string& commandName);

    /**
     * @brief Logging client receives
     * @param commandName Назва команди
     */
    static void LogClientReceive(const std::string& commandName);

    /**
     * @brief Logging client sends
     * @param commandName Назва команди
     */
    static void LogClientSend(const std::string& commandName);

    /**
     * @brief Enabling/disabling log into file
     * @param enabled true - enable, false - disable
     * @param filename file name
     */
    static void SetFileLogging(bool enabled, const std::string& filename = "network_log.txt");

    /**
     * @brief Enable/disable loging
     * @param enabled true - enable, false - disable
     */
    static void SetConsoleLogging(bool enabled);

    /**
     * @brief set level of log detalization
     * @param level 0 - minimal, 1 - standart, 2 - detail
     */
    static void SetVerbosityLevel(int level);

private:
    static bool _fileLoggingEnabled;
    static bool _consoleLoggingEnabled;
    static int _verbosityLevel;
    static std::string _logFilename;
    static std::mutex _logMutex;

public:
    /**
     * @brief put message into file
     * @param message message that will be in log
     * @param skipEndl - skip appending next line \\n
     */
    static void WriteLog(const std::string& message, bool skipEndl = false);

    /**
     * @brief Отримати поточний час як рядок
     * @return Рядок з поточним часом у форматі [HH:MM:SS.mmm]
     */
    static std::string GetCurrentTimeStamp();
};
