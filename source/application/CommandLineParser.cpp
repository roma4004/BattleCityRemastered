#include "application/CommandLineParser.h"
#include "network/Endpoints.h"
#include <boost/asio/ip/address.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm>
#include <cstdint>
#include "enums/WindowSide.h"
#include <charconv>
#include <optional>
#include <string_view>

namespace
{
//NOTE: the host alone - the port has its own option, so an IPv6 literal needs no brackets to keep them apart.
//A name is not resolved, so it is rejected
std::optional<std::string> ParseHost(const std::string_view value)
{
	//NOTE: brackets and a glued-on port are refused, not parsed - make_address cannot be the guard, because
	//on Windows it goes through WSAStringToAddress, which takes "1.2.3.4:4321" and "[::1]:5000" and drops the
	//port on the floor. A single colon can only be a port, since the shortest IPv6 literal has two
	if (value.contains('[') || value.contains(']') || std::ranges::count(value, ':') == 1)
	{
		return std::nullopt;
	}

	boost::system::error_code ec;
	const auto parsed{boost::asio::ip::make_address(std::string{value}, ec)};
	if (ec)
	{
		return std::nullopt;
	}

	return parsed.to_string();
}

//NOTE: "auto" and "0" both ask bind for any free port - the word reads better in a script, the number is
//what every other tool spells it. Leaving the option out means the same thing
std::optional<std::uint16_t> ParsePort(const std::string_view value)
{
	if (value == "auto")
	{
		return std::uint16_t{};
	}

	unsigned number{};
	const auto* const last{value.data() + value.size()};
	const auto [ptr, error] = std::from_chars(value.data(), last, number);
	if (error != std::errc{} || ptr != last || number > 65535u)
	{
		return std::nullopt;
	}

	return static_cast<std::uint16_t>(number);
}

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

		if (arg == "--server")
		{
			launchOptions.gameMode = GameMode::PlayAsHost;
			launchOptions.isDemo = false;

			continue;
		}

		if (arg == "--client")
		{
			launchOptions.gameMode = GameMode::PlayAsClient;
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
			launchOptions.serverHost = ParseHost(value);
			if (!launchOptions.serverHost)
			{
				return std::unexpected(ArgError{.arg = std::string{arg},
												.reason = "expected --address=IP, no brackets and no port - the port goes in --port"});
			}
		}
		else if (key == "port")
		{
			launchOptions.serverPort = ParsePort(value);
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
			launchOptions.serverHost = ParseHost(arg.substr(std::string_view{"--address="}.size()));
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

		if (!arg.starts_with("--port="))
		{
			return std::unexpected(ArgError{.arg = std::string{arg},
											.reason = "the server takes --address, --port, --port-file and --help"});
		}

		launchOptions.serverPort = ParsePort(arg.substr(std::string_view{"--port="}.size()));
		if (!launchOptions.serverPort)
		{
			return std::unexpected(ArgError{.arg = std::string{arg},
											.reason = "expected --port=NUMBER or --port=auto"});
		}
	}

	return launchOptions;
}
