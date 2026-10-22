#pragma once

#include <cstdint>
#include <string>

class Log final
{
public:
	enum class Level : std::uint8_t
	{
		Quiet,
		//NOTE: errors only
		Normal,
		Detailed,
		//NOTE: per-frame chatter - network traffic and the like
	};

	//NOTE: console half goes to stderr, file line is tagged ERROR. Never suppressed by the level.
	static void Error(const std::string& message);
	static void Info(const std::string& message);
	//NOTE: only at Detailed. Ask IsDetailed() first when building the message costs something.
	static void Detail(const std::string& message);

	[[nodiscard]] static bool IsDetailed() noexcept;

	static void SetConsole(bool enabled);
	static void SetFile(bool enabled, const std::string& filename = "game_log.txt");
	static void SetLevel(Level level);

	[[nodiscard]] static std::string TimeStamp();

private:
	static void Write(const std::string& message, bool isError);

	static bool _consoleEnabled;
	static bool _fileEnabled;
	static Level _level;
	static std::string _filename;
};
