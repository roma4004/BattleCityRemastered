#include "application/CommandLineParser.h"
#include "application/GameConfig.h"
#include "application/WindowConfig.h"
#include "application/ProjectConfig.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include "TestUtils.h"//NOTE: PrintTo for the Point types

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

//NOTE: guards the loop over argv - an earlier parser looked at argv[1] only and would drop the second flag
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

	const ProjectConfig projectConfig{"unused.ini", true};
	WindowConfig windowConfig{projectConfig};
	const UPoint iniSize = windowConfig.size;
	windowConfig.Apply(posOnly);

	EXPECT_EQ(windowConfig.pos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(windowConfig.size, iniSize);
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
	const ProjectConfig projectConfig{"unused.ini", true};
	WindowConfig windowConfig{projectConfig};
	windowConfig.Apply(Parse({"pos=10,20", "size=1024,768"}));

	EXPECT_EQ(windowConfig.pos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(windowConfig.size, (UPoint{.x = 1024u, .y = 768u}));
	EXPECT_TRUE(windowConfig.hasExplicitPos);
}

//NOTE: the other half of Apply - an empty optional must leave the ini values in place, not overwrite them with defaults
TEST(CommandLineParserTest, ApplyLeavesConfigAloneWithoutArguments)
{
	const ProjectConfig projectConfig{"unused.ini", true};
	WindowConfig windowConfig{projectConfig};
	const UPoint iniPos = windowConfig.pos;
	const UPoint iniSize = windowConfig.size;

	GameConfig gameConfig{};
	windowConfig.Apply(Parse({}));
	gameConfig.Apply(Parse({}));

	EXPECT_EQ(windowConfig.pos, iniPos);
	EXPECT_EQ(windowConfig.size, iniSize);
	EXPECT_FALSE(windowConfig.hasExplicitPos);
	EXPECT_FALSE(gameConfig.skipIntroMusic);
}

//NOTE: pos/size override this session only - they must not reach the ini tree, which is what SaveIni writes.
//Guards the planned "save window last position" TODO in GameConfig.cpp from persisting them by accident.
TEST(CommandLineParserTest, ApplyDoesNotWriteBackToTheIni)
{
	const ProjectConfig projectConfig{"unused.ini", true};
	WindowConfig windowConfig{projectConfig};
	windowConfig.Apply(Parse({"pos=10,20", "size=1024,768"}));

	EXPECT_EQ(projectConfig.Get<unsigned>("Window.width", 0u), 800u);
	EXPECT_EQ(projectConfig.Get<unsigned>("Window.height", 0u), 600u);
	EXPECT_EQ(projectConfig.Get<unsigned>("Window.posX", 0u), 100u);
	EXPECT_EQ(projectConfig.Get<unsigned>("Window.posY", 0u), 100u);
}

//NOTE: pins the order inside Apply - the offset is half a window wide, so size must be applied before it
TEST(CommandLineParserTest, HostOffsetUsesTheSizeFromTheCommandLine)
{
	const ProjectConfig projectConfig{"unused.ini", true};
	WindowConfig windowConfig{projectConfig};
	windowConfig.Apply(Parse({"host", "size=1024,768"}));

	EXPECT_EQ(windowConfig.posOffset.x, static_cast<size_t>(0) - 1024u / 2u);
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

	[[nodiscard]] const std::filesystem::path& Path() const { return _path; }
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

TEST(ProjectConfigTest, MissingFileIsWrittenAndNotReported)
{
	const TempIni ini{"battlecity_missing.ini"};

	{
		const ProjectConfig projectConfig{ini.Path()};
		EXPECT_FALSE(projectConfig.LoadError().has_value());
	}

	EXPECT_TRUE(ini.Exists());
}

//NOTE: the file used to be replaced by defaults, erasing the very line that needed fixing
TEST(ProjectConfigTest, UnparseableFileIsReportedAndLeftUntouched)
{
	constexpr std::string_view broken{"[Window]\nwidth=800\n=nokey\n"};//NOTE: boost: "key expected", line 3
	const TempIni ini{"battlecity_broken.ini", broken};

	{
		const ProjectConfig projectConfig{ini.Path()};
		EXPECT_EQ(projectConfig.LoadError().value_or(ConfigError{}).line, 3u);

		const WindowConfig windowConfig{projectConfig};
		EXPECT_EQ(windowConfig.size, (UPoint{.x = 800u, .y = 600u}));//NOTE: defaults, not the file's 800
	}

	EXPECT_EQ(ini.Read(), broken);
}

//NOTE: the size half of the flag pair - what keeps a one-off size= out of the ini on the way out
TEST(CommandLineParserTest, ApplyMarksAnExplicitSizeOnItsOwn)
{
	const ProjectConfig projectConfig{"unused.ini", true};
	WindowConfig windowConfig{projectConfig};
	windowConfig.Apply(Parse({"size=1024,768"}));

	EXPECT_TRUE(windowConfig.hasExplicitSize);
	EXPECT_FALSE(windowConfig.hasExplicitPos);
}

//NOTE: pos and size are decided apart - passing one must not stop the other from being remembered
TEST(CommandLineParserTest, OnlyTheOverriddenHalfIsKeptOutOfTheIni)
{
	const ProjectConfig projectConfig{"unused.ini", true};

	WindowConfig plain{projectConfig};
	plain.Apply(Parse({}));
	EXPECT_FALSE(plain.hasExplicitPos);
	EXPECT_FALSE(plain.hasExplicitSize);

	WindowConfig posOnly{projectConfig};
	posOnly.Apply(Parse({"pos=10,20"}));
	EXPECT_TRUE(posOnly.hasExplicitPos);
	EXPECT_FALSE(posOnly.hasExplicitSize);

	WindowConfig sizeOnly{projectConfig};
	sizeOnly.Apply(Parse({"size=1024,768"}));
	EXPECT_FALSE(sizeOnly.hasExplicitPos);
	EXPECT_TRUE(sizeOnly.hasExplicitSize);
}

//NOTE: both processes share one config.ini and both windows sit at an offset - whichever exits last
//would leave the other's launch position behind
TEST(CommandLineParserTest, NetworkModesNeverPersistTheWindowState)
{
	const ProjectConfig projectConfig{"unused.ini", true};

	//NOTE: the predicate moved to SDL_Config - what a config can answer is the half below: both windows
	//are placed by an offset, and an offset window position is what must not reach the ini
	for (const auto* const mode: {"host", "client"})
	{
		WindowConfig windowConfig{projectConfig};
		windowConfig.Apply(Parse({mode}));

		EXPECT_NE(windowConfig.posOffset.x, 0u) << mode;
	}
}

//NOTE: drives the centering branch in SDL_Config::InitRender - a run with nothing saved to restore
TEST(ProjectConfigTest, MissingAndUnparseableFilesBothCountAsFresh)
{
	const TempIni missing{"battlecity_fresh_missing.ini"};
	EXPECT_TRUE(ProjectConfig{missing.Path()}.IsFreshIni());

	const TempIni broken{"battlecity_fresh_broken.ini", "[Window]\n=nokey\n"};
	EXPECT_TRUE(ProjectConfig{broken.Path()}.IsFreshIni());
}

TEST(ProjectConfigTest, AReadableFileIsNotFreshAndCenteringIsOptIn)
{
	const TempIni ini{"battlecity_saved.ini", "[Window]\nposX=340\nposY=180\n"};

	const ProjectConfig projectConfig{ini.Path()};
	EXPECT_FALSE(projectConfig.IsFreshIni());
	EXPECT_FALSE(projectConfig.IsCenterOnStart());

	const WindowConfig windowConfig{projectConfig};
	EXPECT_EQ(windowConfig.pos, (UPoint{.x = 340u, .y = 180u}));
}

TEST(ProjectConfigTest, CenterOnStartIsReadBackFromTheFile)
{
	const TempIni ini{"battlecity_centered.ini", "[Window]\ncenterOnStart=true\n"};

	EXPECT_TRUE(ProjectConfig{ini.Path()}.IsCenterOnStart());
}
