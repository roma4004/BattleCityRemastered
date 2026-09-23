#include "application/ConsoleCommand.h"
#include "enums/PlayerSlot.h"
#include "utils/Log.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <string>
#include <variant>

// a command that takes no argument
TEST(ConsoleCommandTest, ABareCommandIsRecognised)
{
	const auto command{ParseConsoleCommand("/restart")};

	ASSERT_TRUE(command.has_value()) << command.error();
	EXPECT_TRUE(std::holds_alternative<RestartCommand>(*command));
}

// the pair that parses into one command and differs only by the flag
TEST(ConsoleCommandTest, CloseAndOpenSayWhetherClientsAreTaken)
{
	const auto close{ParseConsoleCommand("/close")};
	const auto open{ParseConsoleCommand("/open")};

	ASSERT_TRUE(close.has_value() && open.has_value());
	EXPECT_FALSE(std::get<AcceptClientsCommand>(*close).isAccepting);
	EXPECT_TRUE(std::get<AcceptClientsCommand>(*open).isAccepting);
}

// the argument is a seat, and it reaches the command as one
TEST(ConsoleCommandTest, KickNamesTheSeat)
{
	const auto command{ParseConsoleCommand("/kick p2")};

	ASSERT_TRUE(command.has_value()) << command.error();
	EXPECT_EQ(std::get<KickCommand>(*command).slot, PlayerSlot::P2);
}

// leading spaces, a tab between the words, a trailing carriage return - a line typed into a console
// arrives with whatever the terminal added, and still parses
TEST(ConsoleCommandTest, BlanksAroundTheWordsDoNotMatter)
{
	const auto command{ParseConsoleCommand("  /log\tdetailed\r")};

	ASSERT_TRUE(command.has_value()) << command.error();
	EXPECT_EQ(std::get<LogLevelCommand>(*command).level, Log::Level::Detailed);
}

//NOTE: a typo is refused whole - half-obeying "/exit now" or "/kick p3" would be worse than a message
TEST(ConsoleCommandTest, AMistypedLineIsRefusedWithAReason)
{
	constexpr std::array kMistyped{"", "/unknown", "restart", "/kick", "/kick p3", "/log loud", "/exit now",
								   "/kick p1 p2"};

	std::ranges::for_each(kMistyped, [](const char* line)
	{
		const auto command{ParseConsoleCommand(line)};
		ASSERT_FALSE(command.has_value()) << line;
		EXPECT_FALSE(command.error().empty()) << line;
	});
}

// the console names a level, not a file - the path is built for it
TEST(ConsoleCommandTest, MapNamesTheLevel)
{
	const auto command{ParseConsoleCommand("/map level2")};

	ASSERT_TRUE(command.has_value()) << command.error();
	EXPECT_EQ(std::get<MapCommand>(*command).name, "level2");
	EXPECT_EQ(MapPathForName("level2"), "Resources/Maps/level2.map");
}

// a name is a name: a path would let the console reach outside the maps folder
TEST(ConsoleCommandTest, MapRefusesAPathOrASuffix)
{
	EXPECT_FALSE(ParseConsoleCommand("/map ../secrets/level1").has_value());
	EXPECT_FALSE(ParseConsoleCommand("/map level2.map").has_value());
	EXPECT_FALSE(ParseConsoleCommand(R"(/map maps\level2)").has_value());
}

// the command carries the level to load, so a bare /map is an error rather than a reload
TEST(ConsoleCommandTest, MapWithoutANameIsRefused)
{
	const auto command{ParseConsoleCommand("/map")};

	EXPECT_FALSE(command.has_value());
}
