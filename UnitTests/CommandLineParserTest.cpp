#include "application/CommandLineParser.h"
#include "application/GameConfig.h"
#include "application/WindowConfig.h"
#include "application/ProjectConfig.h"
#include "enums/WindowSide.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <optional>
#include <vector>
#include "TestUtils.h"//NOTE: PrintTo for the Point types

namespace
{
//NOTE: argv[0] is the exe name, the parser starts at [1]
std::expected<LaunchOptions, ArgError> ParseRaw(const std::initializer_list<const char*> args)
{
	std::vector<const char*> argv{"BattleCityRemastered.exe"};
	argv.insert(argv.end(), args);

	return CommandLineParser::Parse(static_cast<int>(argv.size()), argv.data());
}

//NOTE: a rejection here is a bug in the test's own arguments, not a case under test
LaunchOptions Parse(const std::initializer_list<const char*> args)
{
	const auto options = ParseRaw(args);
	EXPECT_TRUE(options.has_value()) << (options ? "" : options.error().reason);

	return options.value_or(LaunchOptions{});
}

std::optional<ArgError> ParseError(const std::initializer_list<const char*> args)
{
	const auto options = ParseRaw(args);
	if (options)
	{
		return std::nullopt;
	}

	return options.error();
}
}//namespace

class CommandLineParserTest : public testing::Test
{
protected:
	//NOTE: the path is never opened - the second argument fills the defaults and skips the disk
	const ProjectConfig _projectConfig{"unused.ini", true};
	WindowConfig _windowConfig{_projectConfig};

	[[nodiscard]] WindowSide SideOf(const std::initializer_list<const char*> args) const
	{
		WindowConfig windowConfig{_projectConfig};
		windowConfig.Apply(Parse(args));

		return windowConfig.side;
	}
};

//NOTE: plain launch must stay untouched - every option here is a deliberate override, not a default
TEST_F(CommandLineParserTest, NoArgumentsGivesDemoPhase)
{
	const auto options = Parse({});

	EXPECT_TRUE(options.isDemo);
	EXPECT_EQ(options.gameMode, GameMode::CoopWithBot);
	EXPECT_FALSE(options.skipIntroMusic);
	EXPECT_FALSE(options.windowPos.has_value());
	EXPECT_FALSE(options.windowSize.has_value());
	EXPECT_FALSE(options.windowSide.has_value());
}

//NOTE: Rider run configs pass "host", the scripts pass "-host" - ends_with must cover both
TEST_F(CommandLineParserTest, GameModeAcceptsBareAndDashedForms)
{
	EXPECT_EQ(Parse({"host"}).gameMode, GameMode::PlayAsHost);
	EXPECT_EQ(Parse({"-host"}).gameMode, GameMode::PlayAsHost);
	EXPECT_EQ(Parse({"client"}).gameMode, GameMode::PlayAsClient);
	EXPECT_EQ(Parse({"--client"}).gameMode, GameMode::PlayAsClient);
}

//NOTE: guards the loop over argv - an earlier parser looked at argv[1] only and would drop the second flag
TEST_F(CommandLineParserTest, SkipIntroCombinesWithGameMode)
{
	const auto options = Parse({"client", "skipintro"});

	EXPECT_EQ(options.gameMode, GameMode::PlayAsClient);
	EXPECT_TRUE(options.skipIntroMusic);
}

//NOTE: the happy path for key=value, both keys at once
TEST_F(CommandLineParserTest, WindowPosAndSizeAreParsed)
{
	const auto options = Parse({"pos=10,20", "size=1024,768"});

	ASSERT_TRUE(options.windowPos.has_value());
	ASSERT_TRUE(options.windowSize.has_value());
	EXPECT_EQ(*options.windowPos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(*options.windowSize, (UPoint{.x = 1024u, .y = 768u}));
}

//NOTE: the two keys are independent optionals - one may be given without the other,
//and the one left out keeps its ini value
TEST_F(CommandLineParserTest, EachWindowOptionIsIndependent)
{
	const auto sizeOnly = Parse({"size=1024,768"});
	EXPECT_TRUE(sizeOnly.windowSize.has_value());
	EXPECT_FALSE(sizeOnly.windowPos.has_value());

	const auto posOnly = Parse({"pos=10,20"});
	EXPECT_TRUE(posOnly.windowPos.has_value());
	EXPECT_FALSE(posOnly.windowSize.has_value());

	const UPoint iniSize = _windowConfig.size;
	_windowConfig.Apply(posOnly);

	EXPECT_EQ(_windowConfig.pos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(_windowConfig.size, iniSize);
}

//NOTE: from_chars must consume the whole field - a partial parse would silently accept "800x600" as 800
TEST_F(CommandLineParserTest, MalformedPointIsRejected)
{
	//NOTE: an empty left side means it was accepted - one compare covers both rejection and naming
	for (const char* arg: {"size=800x600", "size=800,60a", "size=800,", "pos=,20", "pos=-10,20"})
	{
		EXPECT_EQ(ParseError({arg}).value_or(ArgError{}).arg, arg);
	}
}

//NOTE: zero parses fine but makes an unusable window, so it is rejected separately from malformed input
TEST_F(CommandLineParserTest, ZeroWindowSizeIsRejected)
{
	for (const char* arg: {"size=0,600", "size=800,0"})
	{
		EXPECT_EQ(ParseError({arg}).value_or(ArgError{}).arg, arg);
	}
}

TEST_F(CommandLineParserTest, ArgumentsAfterABadOneAreNotParsed)
{
	EXPECT_EQ(ParseError({"host", "size=800x600", "skipintro"}).value_or(ArgError{}).arg, "size=800x600");
}

//NOTE: values reach the config, and an explicit pos raises the flag that stops monitor centering in SDL_Config
TEST_F(CommandLineParserTest, ApplyOverridesConfigAndPinsPosition)
{
	_windowConfig.Apply(Parse({"pos=10,20", "size=1024,768"}));

	EXPECT_EQ(_windowConfig.pos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(_windowConfig.size, (UPoint{.x = 1024u, .y = 768u}));
	EXPECT_TRUE(_windowConfig.hasExplicitPos);
}

//NOTE: the other half of Apply - an empty optional must leave the ini values in place, not overwrite them with defaults
TEST_F(CommandLineParserTest, ApplyLeavesConfigAloneWithoutArguments)
{
	const UPoint iniPos = _windowConfig.pos;
	const UPoint iniSize = _windowConfig.size;

	GameConfig gameConfig{};
	_windowConfig.Apply(Parse({}));
	gameConfig.Apply(Parse({}));

	EXPECT_EQ(_windowConfig.pos, iniPos);
	EXPECT_EQ(_windowConfig.size, iniSize);
	EXPECT_FALSE(_windowConfig.hasExplicitPos);
	EXPECT_FALSE(gameConfig.skipIntroMusic);
}

//NOTE: pos/size override this session only - they must not reach the ini tree, which is what SaveIni writes.
//Guards the planned "save window last position" TODO in GameConfig.cpp from persisting them by accident.
TEST_F(CommandLineParserTest, ApplyDoesNotWriteBackToTheIni)
{
	_windowConfig.Apply(Parse({"pos=10,20", "size=1024,768"}));

	EXPECT_EQ(_projectConfig.Get<unsigned>("Window.width", 0u), 800u);
	EXPECT_EQ(_projectConfig.Get<unsigned>("Window.height", 0u), 600u);
	EXPECT_EQ(_projectConfig.Get<unsigned>("Window.posX", 0u), 100u);
	EXPECT_EQ(_projectConfig.Get<unsigned>("Window.posY", 0u), 100u);
}

//NOTE: a pair launched from one machine must not open on top of itself
TEST_F(CommandLineParserTest, HostAndClientTakeOppositeHalves)
{
	EXPECT_EQ(SideOf({}), WindowSide::Center);
	EXPECT_EQ(SideOf({"host"}), WindowSide::Left);
	EXPECT_EQ(SideOf({"client"}), WindowSide::Right);
}

//NOTE: with a dedicated server both processes are clients, so the mode cannot tell them apart
TEST_F(CommandLineParserTest, AnExplicitSideWinsOverTheGameMode)
{
	EXPECT_EQ(SideOf({"client", "side=left"}), WindowSide::Left);
	EXPECT_EQ(SideOf({"host", "side=right"}), WindowSide::Right);
}

//NOTE: the half is a direction, not a distance - reading pixels overlapped the pair when scaled
TEST_F(CommandLineParserTest, TheSideDoesNotDependOnTheWindowSize)
{
	EXPECT_EQ(SideOf({"client", "size=1024,768"}), SideOf({"client"}));
	EXPECT_EQ(SideOf({"host", "size=1024,768"}), SideOf({"host"}));
}

TEST_F(CommandLineParserTest, MalformedSideIsRejected)
{
	for (const char* arg: {"side=middle", "side=", "side=LEFT"})
	{
		EXPECT_EQ(ParseError({arg}).value_or(ArgError{}).arg, arg);
	}
}

//NOTE: the size half of the flag pair - what keeps a one-off size= out of the ini on the way out
TEST_F(CommandLineParserTest, ApplyMarksAnExplicitSizeOnItsOwn)
{
	_windowConfig.Apply(Parse({"size=1024,768"}));

	EXPECT_TRUE(_windowConfig.hasExplicitSize);
	EXPECT_FALSE(_windowConfig.hasExplicitPos);
}

//NOTE: pos and size are decided apart - passing one must not stop the other from being remembered
TEST_F(CommandLineParserTest, OnlyTheOverriddenHalfIsKeptOutOfTheIni)
{
	WindowConfig plain{_projectConfig};
	plain.Apply(Parse({}));
	EXPECT_FALSE(plain.hasExplicitPos);
	EXPECT_FALSE(plain.hasExplicitSize);

	WindowConfig posOnly{_projectConfig};
	posOnly.Apply(Parse({"pos=10,20"}));
	EXPECT_TRUE(posOnly.hasExplicitPos);
	EXPECT_FALSE(posOnly.hasExplicitSize);

	WindowConfig sizeOnly{_projectConfig};
	sizeOnly.Apply(Parse({"size=1024,768"}));
	EXPECT_FALSE(sizeOnly.hasExplicitPos);
	EXPECT_TRUE(sizeOnly.hasExplicitSize);
}

//NOTE: both processes share one config.ini and both windows sit at an offset - whichever exits last
//would leave the other's launch position behind
TEST_F(CommandLineParserTest, NetworkModesNeverPersistTheWindowState)
{
	//NOTE: the predicate moved to SDL_Config - what a config can answer is the half below: both windows
	//are placed by an offset, and an offset window position is what must not reach the ini
	for (const auto* const mode: {"host", "client"})
	{
		WindowConfig windowConfig{_projectConfig};
		windowConfig.Apply(Parse({mode}));

		EXPECT_NE(windowConfig.side, WindowSide::Center) << mode;
	}
}
