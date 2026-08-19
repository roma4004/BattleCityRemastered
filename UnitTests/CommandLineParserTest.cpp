#include "application/CommandLineParser.h"
#include "application/GameConfig.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
//NOTE: argv[0] is the exe name, the parser starts at [1]
std::expected<LaunchOptions, ArgError> ParseRaw(const std::initializer_list<const char*> args)
{
	std::vector<const char*> argv{"BattleCity_remastered.exe"};
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
TEST(CommandLineParserTest, MalformedPointIsRejected)
{
	//NOTE: an empty left side means it was accepted - one compare covers both rejection and naming
	for (const char* arg: {"size=800x600", "size=800,60a", "size=800,", "pos=,20", "pos=-10,20"})
	{
		EXPECT_EQ(ParseError({arg}).value_or(ArgError{}).arg, arg);
	}
}

//NOTE: zero parses fine but makes an unusable window, so it is rejected separately from malformed input
TEST(CommandLineParserTest, ZeroWindowSizeIsRejected)
{
	for (const char* arg: {"size=0,600", "size=800,0"})
	{
		EXPECT_EQ(ParseError({arg}).value_or(ArgError{}).arg, arg);
	}
}

TEST(CommandLineParserTest, ArgumentsAfterABadOneAreNotParsed)
{
	EXPECT_EQ(ParseError({"host", "size=800x600", "skipintro"}).value_or(ArgError{}).arg, "size=800x600");
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

namespace
{
//NOTE: the file's fate is the behaviour under test, so unlike MapLoader these cases need a real one
class TempIni final
{
	std::filesystem::path _path;

public:
	explicit TempIni(const std::string_view name, const std::string_view contents = {})
		: _path(std::filesystem::temp_directory_path() / name)
	{
		std::filesystem::remove(_path);
		if (!contents.empty())
		{
			std::ofstream{_path} << contents;
		}
	}

	TempIni(const TempIni&) = delete;
	TempIni& operator=(const TempIni&) = delete;

	~TempIni() { std::filesystem::remove(_path); }

	[[nodiscard]] std::string Path() const { return _path.string(); }
	[[nodiscard]] bool Exists() const { return std::filesystem::exists(_path); }

	[[nodiscard]] std::string Read() const
	{
		const std::ifstream file{_path};
		std::ostringstream contents;
		contents << file.rdbuf();

		return contents.str();
	}
};
}//namespace

TEST(GameConfigTest, MissingFileIsWrittenAndNotReported)
{
	const TempIni ini{"battlecity_missing.ini"};

	{
		const GameConfig gameConfig{ini.Path()};
		EXPECT_FALSE(gameConfig.LoadError().has_value());
	}

	EXPECT_TRUE(ini.Exists());
}

//NOTE: the file used to be replaced by defaults, erasing the very line that needed fixing
TEST(GameConfigTest, UnparseableFileIsReportedAndLeftUntouched)
{
	constexpr std::string_view broken{"[Window]\nwidth=800\n=nokey\n"};//NOTE: boost: "key expected", line 3
	const TempIni ini{"battlecity_broken.ini", broken};

	{
		const GameConfig gameConfig{ini.Path()};
		EXPECT_EQ(gameConfig.LoadError().value_or(ConfigError{}).line, 3u);
		EXPECT_EQ(gameConfig.windowSize, (UPoint{.x = 800u, .y = 600u}));//NOTE: defaults, not the file's 800
	}

	EXPECT_EQ(ini.Read(), broken);
}
