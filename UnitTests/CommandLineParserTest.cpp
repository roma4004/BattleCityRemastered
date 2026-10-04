#include "application/CommandLineParser.h"
#include "application/GameConfig.h"
#include "application/WindowConfig.h"
#include "application/ProjectConfig.h"
#include "enums/MatchRules.h"
#include "enums/WindowSide.h"
#include "network/Endpoints.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
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
	const auto options{ParseRaw(args)};
	EXPECT_TRUE(options.has_value()) << (options ? "" : options.error().reason);

	return options.value_or(LaunchOptions{});
}

std::optional<ArgError> ParseError(const std::initializer_list<const char*> args)
{
	const auto options{ParseRaw(args)};
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
	const auto options{Parse({})};

	EXPECT_TRUE(options.isDemo);
	EXPECT_EQ(options.gameMode, GameMode::CoopWithBot);
	EXPECT_FALSE(options.isMuted);
	EXPECT_FALSE(options.windowPos.has_value());
	EXPECT_FALSE(options.windowSize.has_value());
	EXPECT_FALSE(options.windowSide.has_value());
}

// the mode is an option of its own, not a bare word
TEST_F(CommandLineParserTest, GameModeIsGivenByItsOwnOption)
{
	EXPECT_EQ(Parse({"--server"}).gameMode, GameMode::PlayAsHost);
	EXPECT_EQ(Parse({"--client"}).gameMode, GameMode::PlayAsClient);
	EXPECT_EQ(Parse({"--ffa"}).gameMode, GameMode::FreeForAll);
	EXPECT_EQ(Parse({"--2p-ffa"}).gameMode, GameMode::TwoPlayersFreeForAll);
}

//NOTE: an earlier parser matched by suffix, so every form below quietly passed for a game mode
TEST_F(CommandLineParserTest, AnOptionNameIsMatchedWhole)
{
	for (const char* arg: {"server", "-server", "myserver", "--servers", "--client=1", "--bogus", "--bogus=1"})
	{
		EXPECT_EQ(ParseError({arg}).value_or(ArgError{}).arg, arg);
	}
}

// help answers to both the long and the short spelling
TEST_F(CommandLineParserTest, HelpIsAskedForByEitherForm)
{
	EXPECT_TRUE(Parse({"--help"}).isHelpRequested);
	EXPECT_TRUE(Parse({"-h"}).isHelpRequested);
	EXPECT_FALSE(Parse({"--server"}).isHelpRequested);
}

//NOTE: guards the loop over argv - an earlier parser looked at argv[1] only and would drop the second flag
TEST_F(CommandLineParserTest, MuteCombinesWithGameMode)
{
	const auto options{Parse({"--client", "--mute"})};

	EXPECT_EQ(options.gameMode, GameMode::PlayAsClient);
	EXPECT_TRUE(options.isMuted);
}

// the window place and size are read as points
TEST_F(CommandLineParserTest, WindowPosAndSizeAreParsed)
{
	const auto options{Parse({"--pos=10,20", "--size=1024,768"})};

	ASSERT_TRUE(options.windowPos.has_value());
	ASSERT_TRUE(options.windowSize.has_value());
	EXPECT_EQ(*options.windowPos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(*options.windowSize, (UPoint{.x = 1024u, .y = 768u}));
}

//NOTE: the two keys are independent optionals - one may be given without the other,
//and the one left out keeps its ini value
TEST_F(CommandLineParserTest, EachWindowOptionIsIndependent)
{
	const auto sizeOnly{Parse({"--size=1024,768"})};
	EXPECT_TRUE(sizeOnly.windowSize.has_value());
	EXPECT_FALSE(sizeOnly.windowPos.has_value());

	const auto posOnly{Parse({"--pos=10,20"})};
	EXPECT_TRUE(posOnly.windowPos.has_value());
	EXPECT_FALSE(posOnly.windowSize.has_value());

	const UPoint iniSize{_windowConfig.size};
	_windowConfig.Apply(posOnly);

	EXPECT_EQ(_windowConfig.pos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(_windowConfig.size, iniSize);
}

//NOTE: from_chars must consume the whole field - a partial parse would silently accept "800x600" as 800
TEST_F(CommandLineParserTest, MalformedPointIsRejected)
{
	//NOTE: an empty left side means it was accepted - one compare covers both rejection and naming
	for (const char* arg: {"--size=800x600", "--size=800,60a", "--size=800,", "--pos=,20", "--pos=-10,20"})
	{
		EXPECT_EQ(ParseError({arg}).value_or(ArgError{}).arg, arg);
	}
}

//NOTE: zero parses fine but makes an unusable window, so it is rejected separately from malformed input
TEST_F(CommandLineParserTest, ZeroWindowSizeIsRejected)
{
	for (const char* arg: {"--size=0,600", "--size=800,0"})
	{
		EXPECT_EQ(ParseError({arg}).value_or(ArgError{}).arg, arg);
	}
}

// parsing stops at the first bad option, so what follows is not read
TEST_F(CommandLineParserTest, ArgumentsAfterABadOneAreNotParsed)
{
	EXPECT_EQ(ParseError({"--server", "--size=800x600", "--mute"}).value_or(ArgError{}).arg, "--size=800x600");
}

//NOTE: values reach the config, and an explicit pos raises the flag that stops monitor centering in SDL_Config
TEST_F(CommandLineParserTest, ApplyOverridesConfigAndPinsPosition)
{
	_windowConfig.Apply(Parse({"--pos=10,20", "--size=1024,768"}));

	EXPECT_EQ(_windowConfig.pos, (UPoint{.x = 10u, .y = 20u}));
	EXPECT_EQ(_windowConfig.size, (UPoint{.x = 1024u, .y = 768u}));
	EXPECT_TRUE(_windowConfig.hasExplicitPos);
}

//NOTE: the other half of Apply - an empty optional must leave the ini values in place, not overwrite them with defaults
TEST_F(CommandLineParserTest, ApplyLeavesConfigAloneWithoutArguments)
{
	const UPoint iniPos{_windowConfig.pos};
	const UPoint iniSize{_windowConfig.size};

	GameConfig gameConfig{};
	_windowConfig.Apply(Parse({}));
	gameConfig.Apply(Parse({}));

	EXPECT_EQ(_windowConfig.pos, iniPos);
	EXPECT_EQ(_windowConfig.size, iniSize);
	EXPECT_FALSE(_windowConfig.hasExplicitPos);
	EXPECT_FALSE(gameConfig.isMuted);
}

//NOTE: pos/size override this session only - they must not reach the ini tree, which is what SaveIni writes.
TEST_F(CommandLineParserTest, ApplyDoesNotWriteBackToTheIni)
{
	_windowConfig.Apply(Parse({"--pos=10,20", "--size=1024,768"}));

	EXPECT_EQ(_projectConfig.Get<unsigned>("Window.width", 0u), 800u);
	EXPECT_EQ(_projectConfig.Get<unsigned>("Window.height", 0u), 600u);
	EXPECT_EQ(_projectConfig.Get<unsigned>("Window.posX", 0u), 100u);
	EXPECT_EQ(_projectConfig.Get<unsigned>("Window.posY", 0u), 100u);
}

//NOTE: a pair launched from one machine must not open on top of itself
TEST_F(CommandLineParserTest, ServerAndClientTakeOppositeHalves)
{
	EXPECT_EQ(SideOf({}), WindowSide::Center);
	EXPECT_EQ(SideOf({"--server"}), WindowSide::Left);
	EXPECT_EQ(SideOf({"--client"}), WindowSide::Right);
}

//NOTE: with a dedicated server both processes are clients, so the mode cannot tell them apart
TEST_F(CommandLineParserTest, AnExplicitSideWinsOverTheGameMode)
{
	EXPECT_EQ(SideOf({"--client", "--side=left"}), WindowSide::Left);
	EXPECT_EQ(SideOf({"--server", "--side=right"}), WindowSide::Right);
}

//NOTE: the half is a direction, not a distance - reading pixels overlapped the pair when scaled
TEST_F(CommandLineParserTest, TheSideDoesNotDependOnTheWindowSize)
{
	EXPECT_EQ(SideOf({"--client", "--size=1024,768"}), SideOf({"--client"}));
	EXPECT_EQ(SideOf({"--server", "--size=1024,768"}), SideOf({"--server"}));
}

// a side that is neither server nor client is refused
TEST_F(CommandLineParserTest, MalformedSideIsRejected)
{
	for (const char* arg: {"--side=middle", "--side=", "--side=LEFT"})
	{
		EXPECT_EQ(ParseError({arg}).value_or(ArgError{}).arg, arg);
	}
}

//NOTE: the size half of the flag pair - what keeps a one-off size= out of the ini on the way out
TEST_F(CommandLineParserTest, ApplyMarksAnExplicitSizeOnItsOwn)
{
	_windowConfig.Apply(Parse({"--size=1024,768"}));

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
	posOnly.Apply(Parse({"--pos=10,20"}));
	EXPECT_TRUE(posOnly.hasExplicitPos);
	EXPECT_FALSE(posOnly.hasExplicitSize);

	WindowConfig sizeOnly{_projectConfig};
	sizeOnly.Apply(Parse({"--size=1024,768"}));
	EXPECT_FALSE(sizeOnly.hasExplicitPos);
	EXPECT_TRUE(sizeOnly.hasExplicitSize);
}

//NOTE: both processes share one config.ini and both windows sit at an offset - whichever exits last
//would leave the other's launch position behind
TEST_F(CommandLineParserTest, NetworkModesNeverPersistTheWindowState)
{
	for (const auto* const mode: {"--server", "--client"})
	{
		WindowConfig windowConfig{_projectConfig};
		windowConfig.Apply(Parse({mode}));

		EXPECT_NE(windowConfig.side, WindowSide::Center) << mode;
	}
}

// give both halves and read them back
TEST_F(CommandLineParserTest, AnAddressAndAPortAreTakenApart)
{
	const auto options{Parse({"--address=192.168.1.5", "--port=4321"})};

	EXPECT_EQ(options.serverHost, "192.168.1.5");
	EXPECT_EQ(options.serverPort, 4321u);
}

// the port has its own option, so nothing has to be cut out of an IPv6 literal and it needs no brackets
TEST_F(CommandLineParserTest, AnIPv6HostNeedsNoBrackets)
{
	const auto options{Parse({"--address=::1", "--port=5000"})};

	EXPECT_EQ(options.serverHost, "::1");
	EXPECT_EQ(options.serverPort, 5000u);
}

// what is not passed stays empty, and the ini value behind it is left alone
TEST_F(CommandLineParserTest, EitherHalfCanBeLeftOut)
{
	EXPECT_FALSE(Parse({"--address=10.0.0.2"}).serverPort.has_value());
	EXPECT_FALSE(Parse({"--port=4321"}).serverHost.has_value());
}

// both spellings of "any free port": the word for a script to read, the number every other tool uses
TEST_F(CommandLineParserTest, AutoAndZeroBothAskForAFreePort)
{
	EXPECT_EQ(Parse({"--port=auto"}).serverPort, 0u);
	EXPECT_EQ(Parse({"--port=0"}).serverPort, 0u);
}

//NOTE: a name is not resolved - and "localhost" once read as the server flag. A host with a port glued on is
//refused outright: "::1:5000" is a valid IPv6 literal of its own, and taking it would dial the wrong machine
TEST_F(CommandLineParserTest, AMalformedAddressIsRejected)
{
	constexpr std::array kMalformed{"--address=localhost", "--address=1.2.3.4:4321", "--address=[::1]:5000",
									"--address=[::1]", "--address=1.2.3.4:", "--address="};

	std::ranges::for_each(kMalformed, [](const char* arg) { EXPECT_TRUE(ParseError({arg}).has_value()) << arg; });
}

// and so is a port that is not a number
TEST_F(CommandLineParserTest, AMalformedPortIsRejected)
{
	constexpr std::array kMalformed{"--port=70000", "--port=12a", "--port=-1", "--port=any", "--port="};

	std::ranges::for_each(kMalformed, [](const char* arg) { EXPECT_TRUE(ParseError({arg}).has_value()) << arg; });
}

// what was parsed has to arrive in the config the game actually dials from
TEST_F(CommandLineParserTest, TheAddressReachesTheGameConfig)
{
	GameConfig gameConfig{};
	gameConfig.Apply(Parse({"--client", "--address=10.0.0.2", "--port=4000"}));

	EXPECT_EQ(gameConfig.serverAddress.host, "10.0.0.2");
	EXPECT_EQ(gameConfig.serverAddress.port, 4000u);
}

// only the half that was passed is written over - the other keeps what the ini gave the config
TEST_F(CommandLineParserTest, TheHalfThatWasNotPassedKeepsTheConfigValue)
{
	GameConfig gameConfig{};
	gameConfig.serverAddress = network::ServerAddress{.host = "10.0.0.9", .port = 7777u};
	gameConfig.Apply(Parse({"--client", "--port=4000"}));

	EXPECT_EQ(gameConfig.serverAddress.host, "10.0.0.9");
	EXPECT_EQ(gameConfig.serverAddress.port, 4000u);
}


namespace
{
std::expected<LaunchOptions, ArgError> ParseServerRaw(const std::initializer_list<const char*> args)
{
	std::vector<const char*> argv{"BattleCityServer.exe"};
	argv.insert(argv.end(), args);

	return CommandLineParser::ParseServer(static_cast<int>(argv.size()), argv.data());
}
}//namespace

// the server set is narrow, so each case is an argument or two and what they parsed into
TEST(ServerCommandLineTest, TheListenAddressIsTaken)
{
	const auto options{ParseServerRaw({"--address=0.0.0.0", "--port=4000"})};

	ASSERT_TRUE(options.has_value()) << options.error().reason;
	EXPECT_EQ(options->serverHost, "0.0.0.0");
	EXPECT_EQ(options->serverPort, 4000u);
}

// the server is the side that can actually take any free port - the game learns which one it got
TEST(ServerCommandLineTest, AFreePortIsAskedForByWordOrByZero)
{
	EXPECT_EQ(ParseServerRaw({"--port=auto"}).value_or(LaunchOptions{}).serverPort, 0u);
	EXPECT_EQ(ParseServerRaw({"--port=0"}).value_or(LaunchOptions{}).serverPort, 0u);
}

// help answers to both the long and the short spelling
TEST(ServerCommandLineTest, HelpIsAskedForByEitherForm)
{
	EXPECT_TRUE(ParseServerRaw({"--help"}).value_or(LaunchOptions{}).isHelpRequested);
	EXPECT_TRUE(ParseServerRaw({"-h"}).value_or(LaunchOptions{}).isHelpRequested);
}

// the game options parse fine in the other exe and do nothing here - taken silently, they would look obeyed
TEST(ServerCommandLineTest, AGameOptionIsRefused)
{
	constexpr std::array kGameOnly{"--server", "--client", "--mute", "--size=800,600", "--pos=0,0", "--side=left"};

	std::ranges::for_each(kGameOnly, [](const char* arg)
	{
		EXPECT_FALSE(ParseServerRaw({arg}).has_value()) << arg;
	});
}

// --seats is the server's
TEST(ServerCommandLineTest, TheSeatCountIsTaken)
{
	EXPECT_EQ(ParseServerRaw({"--seats=4"}).value_or(LaunchOptions{}).seats, 4u);
	EXPECT_FALSE(ParseServerRaw({}).value_or(LaunchOptions{.seats = 3u}).seats.has_value());
}

// and so are its rules - anything but the two is refused
TEST(ServerCommandLineTest, TheRulesAreTaken)
{
	EXPECT_EQ(ParseServerRaw({"--rules=ffa"}).value_or(LaunchOptions{}).rules, MatchRules::FreeForAll);
	EXPECT_EQ(ParseServerRaw({"--rules=classic"}).value_or(LaunchOptions{}).rules, MatchRules::Classic);
	EXPECT_FALSE(ParseServerRaw({"--rules=coop"}).has_value());
}

// the map is named the way the console names it, never by a path
TEST(ServerCommandLineTest, TheMapIsTakenByName)
{
	EXPECT_EQ(ParseServerRaw({"--map=level2"}).value_or(LaunchOptions{}).mapName, "level2");
	for (const char* arg: {"--map=", "--map=../level1", "--map=level1.map", "--map=a\\b", "--map=a b"})
	{
		EXPECT_FALSE(ParseServerRaw({arg}).has_value()) << arg;
	}
}

// and the enemies on the field at once, one to four
TEST(ServerCommandLineTest, TheEnemiesAtOnceAreTakenFromOneToFour)
{
	EXPECT_EQ(ParseServerRaw({"--enemies=1"}).value_or(LaunchOptions{}).enemiesAtOnce, 1u);
	for (const char* arg: {"--enemies=0", "--enemies=5", "--enemies=x", "--enemies="})
	{
		EXPECT_FALSE(ParseServerRaw({arg}).has_value()) << arg;
	}
}

// the bots for the seats nobody took, none to three - one seat is always a player's
TEST(ServerCommandLineTest, TheBotsAreTakenFromNoneToThree)
{
	EXPECT_EQ(ParseServerRaw({"--bots=0"}).value_or(LaunchOptions{}).bots, 0u);
	EXPECT_EQ(ParseServerRaw({"--bots=3"}).value_or(LaunchOptions{}).bots, 3u);
	for (const char* arg: {"--bots=4", "--bots=-1", "--bots=x", "--bots="})
	{
		EXPECT_FALSE(ParseServerRaw({arg}).has_value()) << arg;
	}
}

// and whether the match waits for every seat or starts with whoever is in
TEST(ServerCommandLineTest, TheStartIsTaken)
{
	EXPECT_EQ(ParseServerRaw({"--start=now"}).value_or(LaunchOptions{}).isStartingAtOnce, true);
	EXPECT_EQ(ParseServerRaw({"--start=full"}).value_or(LaunchOptions{}).isStartingAtOnce, false);
	EXPECT_FALSE(ParseServerRaw({"--start=later"}).has_value());
}

// the port is opened on the router only when asked - a server for a LAN party has no business with the router
TEST(ServerCommandLineTest, TheRouterIsAskedOnlyWithUpnp)
{
	EXPECT_TRUE(ParseServerRaw({"--upnp"}).value_or(LaunchOptions{}).isPortForwarded);
	EXPECT_FALSE(ParseServerRaw({"--port=4000"}).value_or(LaunchOptions{.isPortForwarded = true}).isPortForwarded);
	EXPECT_FALSE(ParseServerRaw({"--upnp=yes"}).has_value());
}

// one to four seats only
TEST(ServerCommandLineTest, ASeatCountOutsideOneToFourIsRefused)
{
	for (const char* arg: {"--seats=0", "--seats=5", "--seats=two", "--seats="})
	{
		EXPECT_FALSE(ParseServerRaw({arg}).has_value()) << arg;
	}
}

// the dedicated server refuses the same malformed address the game does
TEST(ServerCommandLineTest, AMalformedAddressIsRejectedHereToo)
{
	EXPECT_FALSE(ParseServerRaw({"--address=localhost"}).has_value());
	EXPECT_FALSE(ParseServerRaw({"--address=1.2.3.4:4321"}).has_value());
	EXPECT_FALSE(ParseServerRaw({"--address"}).has_value());
	EXPECT_FALSE(ParseServerRaw({"--port=70000"}).has_value());
}
