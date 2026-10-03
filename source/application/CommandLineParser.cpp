#include "application/CommandLineParser.h"
#include "network/Endpoints.h"
#include "enums/GameMode.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include "enums/WindowSide.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

namespace
{
//NOTE: each skips the demo and the menu
constexpr std::array<std::pair<std::string_view, GameMode>, 4> kModeFlags{{
		{"--server", GameMode::PlayAsHost},
		{"--client", GameMode::PlayAsClient},
		{"--ffa", GameMode::FreeForAll},
		{"--2p-ffa", GameMode::TwoPlayersFreeForAll}}};

//NOTE: "800,600", fully consumed - "800x600"/"800,60a" rejected
std::optional<UPoint> ParsePoint(const std::string_view value)
{
	const auto separator{value.find(',')};
	if (separator == std::string_view::npos)
	{
		return std::nullopt;
	}

	const auto parseField = [](const std::string_view field, size_t& out)
	{
		//NOTE: 'last' bounds from_chars, no terminator needed
		const auto* const first{field.data()};
		const auto* const last{first + field.size()};
		const auto [ptr, ec] = std::from_chars(first, last, out);
		return ec == std::errc{} && ptr == last;
	};

	UPoint point{};
	if (!parseField(value.substr(0, separator), point.x) || !parseField(value.substr(separator + 1), point.y))
	{
		return std::nullopt;
	}

	return point;
}

std::optional<WindowSide> ParseSide(const std::string_view value)
{
	if (value == "left")
	{
		return WindowSide::Left;
	}

	if (value == "right")
	{
		return WindowSide::Right;
	}

	return std::nullopt;
}
}//namespace

//NOTE: names are compared whole - an unknown one is an error, so a typo is never taken for a dead flag
std::expected<LaunchOptions, ArgError> CommandLineParser::Parse(const int argc, const char* const* argv)
{
	LaunchOptions launchOptions{};

	for (int i = 1; i < argc; ++i)
	{
		const std::string_view arg{argv[i]};
		if (arg == "--help" || arg == "-h")
		{
			launchOptions.isHelpRequested = true;

			continue;
		}

		if (const auto flag{std::ranges::find(kModeFlags, arg, &std::pair<std::string_view, GameMode>::first)};
			flag != kModeFlags.end())
		{
			launchOptions.gameMode = flag->second;
			launchOptions.isDemo = false;

			continue;
		}

		if (arg == "--mute")
		{
			launchOptions.isMuted = true;

			continue;
		}

		const auto separator{arg.find('=')};
		if (!arg.starts_with("--") || separator == std::string_view::npos)
		{
			return std::unexpected(ArgError{.arg = std::string{arg}, .reason = "unknown argument, --help lists them"});
		}

		const std::string_view key{arg.substr(2, separator - 2)};
		const std::string_view value{arg.substr(separator + 1)};
		if (key == "pos")
		{
			launchOptions.windowPos = ParsePoint(value);
			if (!launchOptions.windowPos)
			{
				return std::unexpected(ArgError{.arg = std::string{arg}, .reason = "expected --pos=X,Y"});
			}
		}
		else if (key == "size")
		{
			launchOptions.windowSize = ParsePoint(value);
			if (launchOptions.windowSize && (launchOptions.windowSize->x == 0 || launchOptions.windowSize->y == 0))
			{
				launchOptions.windowSize.reset();
			}

			if (!launchOptions.windowSize)
			{
				return std::unexpected(ArgError{.arg = std::string{arg},
												.reason = "expected --size=WIDTH,HEIGHT with both above zero"});
			}
		}
		else if (key == "address")
		{
			launchOptions.serverHost = network::ParseHost(value);
			if (!launchOptions.serverHost)
			{
				return std::unexpected(ArgError{.arg = std::string{arg},
												.reason = "expected --address=IP, no brackets and no port - the port goes in --port"});
			}
		}
		else if (key == "port")
		{
			launchOptions.serverPort = network::ParsePort(value);
			if (!launchOptions.serverPort)
			{
				return std::unexpected(ArgError{.arg = std::string{arg},
												.reason = "expected --port=NUMBER or --port=auto"});
			}
		}
		else if (key == "side")
		{
			launchOptions.windowSide = ParseSide(value);
			if (!launchOptions.windowSide)
			{
				return std::unexpected(ArgError{.arg = std::string{arg},
												.reason = "expected --side=left or --side=right"});
			}
		}
		else
		{
			return std::unexpected(ArgError{.arg = std::string{arg}, .reason = "unknown option, --help lists them"});
		}
	}

	return launchOptions;
}

std::expected<LaunchOptions, ArgError> CommandLineParser::ParseServer(const int argc, const char* const* argv)
{
	LaunchOptions launchOptions{};

	for (int i = 1; i < argc; ++i)
	{
		const std::string_view arg{argv[i]};
		if (arg == "--help" || arg == "-h")
		{
			launchOptions.isHelpRequested = true;

			continue;
		}

		if (arg.starts_with("--address="))
		{
			launchOptions.serverHost = network::ParseHost(arg.substr(std::string_view{"--address="}.size()));
			if (!launchOptions.serverHost)
			{
				return std::unexpected(ArgError{.arg = std::string{arg},
												.reason = "expected --address=IP, no brackets and no port - the port goes in --port"});
			}

			continue;
		}

		if (arg.starts_with("--port-file="))
		{
			launchOptions.portFilePath = std::string{arg.substr(std::string_view{"--port-file="}.size())};
			if (launchOptions.portFilePath->empty())
			{
				return std::unexpected(ArgError{.arg = std::string{arg}, .reason = "expected --port-file=PATH"});
			}

			continue;
		}

		if (arg.starts_with("--seats="))
		{
			const std::string_view value{arg.substr(std::string_view{"--seats="}.size())};
			std::size_t seats{};
			const auto* const last{value.data() + value.size()};
			if (const auto [ptr, error] = std::from_chars(value.data(), last, seats);
				error != std::errc{} || ptr != last || seats == 0u || seats > kSeatCount)
			{
				return std::unexpected(ArgError{.arg = std::string{arg}, .reason = "expected --seats=1 to --seats=4"});
			}

			launchOptions.seats = seats;

			continue;
		}

		if (arg.starts_with("--rules="))
		{
			const std::string_view value{arg.substr(std::string_view{"--rules="}.size())};
			if (value != "classic" && value != "ffa")
			{
				return std::unexpected(
						ArgError{.arg = std::string{arg}, .reason = "expected --rules=classic or --rules=ffa"});
			}

			launchOptions.rules = value == "ffa" ? MatchRules::FreeForAll : MatchRules::Classic;

			continue;
		}

		if (!arg.starts_with("--port="))
		{
			return std::unexpected(ArgError{.arg = std::string{arg}, .reason = "unknown option, --help lists them"});
		}

		launchOptions.serverPort = network::ParsePort(arg.substr(std::string_view{"--port="}.size()));
		if (!launchOptions.serverPort)
		{
			return std::unexpected(ArgError{.arg = std::string{arg},
											.reason = "expected --port=NUMBER or --port=auto"});
		}
	}

	return launchOptions;
}
