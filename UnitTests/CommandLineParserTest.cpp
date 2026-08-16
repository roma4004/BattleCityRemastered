#include "application/CommandLineParser.h"
#include "application/GameConfig.h"

#include <gtest/gtest.h>

namespace
{
//NOTE: argv[0] is the exe name, the parser starts at [1]
LaunchOptions Parse(const std::initializer_list<const char*> args)
{
	std::vector<const char*> argv{"BattleCity_remastered.exe"};
	argv.insert(argv.end(), args);

	return CommandLineParser::Parse(static_cast<int>(argv.size()), argv.data());
}
}//namespace

//NOTE: plain launch must stay untouched - every option here is a deliberate override, not a default
TEST(CommandLineParserTest, NoArgumentsGivesDemoMode)
{
	const auto options = Parse({});

	EXPECT_EQ(options.gameMode, GameMode::Demo);
	EXPECT_FALSE(options.skipIntroMusic);
	EXPECT_FALSE(options.windowPos.has_value());
	EXPECT_FALSE(options.windowSize.has_value());
}

//NOTE: Rider run configs pass "host", the scripts pass "-host" - ends_with must cover both
TEST(CommandLineParserTest, GameModeAcceptsBareAndDashedForms)
{
	EXPECT_EQ(Parse({"host"}).gameMode, GameMode::PlayAsHost);
	EXPECT_EQ(Parse({"-host"}).gameMode, GameMode::PlayAsHost);
	EXPECT_EQ(Parse({"client"}).gameMode, GameMode::PlayAsClient);
	EXPECT_EQ(Parse({"--client"}).gameMode, GameMode::PlayAsClient);
}

//NOTE: guards the loop over argv - the pre-WI-49 parser looked at argv[1] only and would drop the second flag
TEST(CommandLineParserTest, SkipIntroCombinesWithGameMode)
{
	const auto options = Parse({"client", "skipintro"});

	EXPECT_EQ(options.gameMode, GameMode::PlayAsClient);
	EXPECT_TRUE(options.skipIntroMusic);
}

//NOTE: the happy path for key=value, both keys at once
TEST(CommandLineParserTest, WindowPosAndSizeAreParsed)
{
	const auto options = Parse({"pos=10,20", "size=1024,768"});

	ASSERT_TRUE(options.windowPos.has_value());
	ASSERT_TRUE(options.windowSize.has_value());
	EXPECT_EQ(*options.windowPos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(*options.windowSize, (UPoint{.x = 1024u, .y = 768u}));
}

//NOTE: the two keys are independent optionals - one may be given without the other,
//and the one left out keeps its ini value
TEST(CommandLineParserTest, EachWindowOptionIsIndependent)
{
	const auto sizeOnly = Parse({"size=1024,768"});
	EXPECT_TRUE(sizeOnly.windowSize.has_value());
	EXPECT_FALSE(sizeOnly.windowPos.has_value());

	const auto posOnly = Parse({"pos=10,20"});
	EXPECT_TRUE(posOnly.windowPos.has_value());
	EXPECT_FALSE(posOnly.windowSize.has_value());

	GameConfig gameConfig{"unused.ini", true};
	const UPoint iniSize = gameConfig.windowSize;
	gameConfig.Apply(posOnly);

	EXPECT_EQ(gameConfig.windowPos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(gameConfig.windowSize, iniSize);
}

//NOTE: from_chars must consume the whole field - a partial parse would silently accept "800x600" as 800
TEST(CommandLineParserTest, MalformedPointIsIgnored)
{
	EXPECT_FALSE(Parse({"size=800x600"}).windowSize.has_value());
	EXPECT_FALSE(Parse({"size=800,60a"}).windowSize.has_value());
	EXPECT_FALSE(Parse({"size=800,"}).windowSize.has_value());
	EXPECT_FALSE(Parse({"pos=,20"}).windowPos.has_value());
	EXPECT_FALSE(Parse({"pos=-10,20"}).windowPos.has_value());
}

//NOTE: zero parses fine but makes an unusable window, so it is rejected separately from malformed input
TEST(CommandLineParserTest, ZeroWindowSizeIsRejected)
{
	EXPECT_FALSE(Parse({"size=0,600"}).windowSize.has_value());
	EXPECT_FALSE(Parse({"size=800,0"}).windowSize.has_value());
}

//NOTE: values reach the config, and an explicit pos raises the flag that stops monitor centering in SDL_Config
TEST(CommandLineParserTest, ApplyOverridesConfigAndPinsPosition)
{
	GameConfig gameConfig{"unused.ini", true};
	gameConfig.Apply(Parse({"pos=10,20", "size=1024,768"}));

	EXPECT_EQ(gameConfig.windowPos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(gameConfig.windowSize, (UPoint{.x = 1024u, .y = 768u}));
	EXPECT_TRUE(gameConfig.hasExplicitWindowPos);
}

//NOTE: the other half of Apply - an empty optional must leave the ini values in place, not overwrite them with defaults
TEST(CommandLineParserTest, ApplyLeavesConfigAloneWithoutArguments)
{
	GameConfig gameConfig{"unused.ini", true};
	const UPoint iniPos = gameConfig.windowPos;
	const UPoint iniSize = gameConfig.windowSize;

	gameConfig.Apply(Parse({}));

	EXPECT_EQ(gameConfig.windowPos, iniPos);
	EXPECT_EQ(gameConfig.windowSize, iniSize);
	EXPECT_FALSE(gameConfig.hasExplicitWindowPos);
	EXPECT_FALSE(gameConfig.skipIntroMusic);
}

//NOTE: pos/size override this session only - they must not reach the ini tree, which is what SaveIni writes.
//Guards the planned "save window last position" TODO in GameConfig.cpp from persisting them by accident.
TEST(CommandLineParserTest, ApplyDoesNotWriteBackToTheIni)
{
	GameConfig gameConfig{"unused.ini", true};
	gameConfig.Apply(Parse({"pos=10,20", "size=1024,768"}));

	EXPECT_EQ(gameConfig.Get<unsigned>("Window.width", 0u), 800u);
	EXPECT_EQ(gameConfig.Get<unsigned>("Window.height", 0u), 600u);
	EXPECT_EQ(gameConfig.Get<unsigned>("Window.posX", 0u), 100u);
	EXPECT_EQ(gameConfig.Get<unsigned>("Window.posY", 0u), 100u);
}

//NOTE: pins the order inside Apply - the offset is half a window wide, so size must be applied before it
TEST(CommandLineParserTest, HostOffsetUsesTheSizeFromTheCommandLine)
{
	GameConfig gameConfig{"unused.ini", true};
	gameConfig.Apply(Parse({"host", "size=1024,768"}));

	EXPECT_EQ(gameConfig.windowsPosOffset.x, static_cast<size_t>(0) - 1024u / 2u);
}
