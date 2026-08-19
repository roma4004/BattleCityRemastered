#include "application/CommandLineParser.h"
#include <charconv>
#include <string_view>

namespace
{
//NOTE: "800,600", fully consumed - "800x600"/"800,60a" rejected
std::optional<UPoint> ParsePoint(const std::string_view value)
{
	const auto separator = value.find(',');
	if (separator == std::string_view::npos)
	{
		return std::nullopt;
	}

	const auto parseField = [](const std::string_view field, size_t& out)
	{
		//NOTE: 'last' bounds from_chars, no terminator needed
		const auto* const first = field.data();
		const auto* const last = first + field.size();
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
}//namespace

//NOTE: ends_with - "host" and "-host" both work
std::expected<LaunchOptions, ArgError> CommandLineParser::Parse(const int argc, const char* const* argv)
{
	LaunchOptions launchOptions{};

	for (int i = 1; i < argc; ++i)
	{
		const std::string_view arg{argv[i]};
		if (arg.ends_with("host"))
		{
			launchOptions.gameMode = GameMode::PlayAsHost;
		}
		else if (arg.ends_with("client"))
		{
			launchOptions.gameMode = GameMode::PlayAsClient;
		}
		else if (arg.ends_with("skipintro"))
		{
			launchOptions.skipIntroMusic = true;
		}
		else if (const auto separator = arg.find('='); separator != std::string_view::npos)
		{
			const std::string_view key = arg.substr(0, separator);
			const std::string_view value = arg.substr(separator + 1);

			if (key.ends_with("pos"))
			{
				launchOptions.windowPos = ParsePoint(value);
				if (!launchOptions.windowPos)
				{
					return std::unexpected(ArgError{.arg = std::string{arg}, .reason = "expected pos=X,Y"});
				}
			}
			else if (key.ends_with("size"))
			{
				launchOptions.windowSize = ParsePoint(value);
				if (launchOptions.windowSize && (launchOptions.windowSize->x == 0 || launchOptions.windowSize->y == 0))
				{
					launchOptions.windowSize.reset();//NOTE: a zero-sized window is worse than none
				}

				if (!launchOptions.windowSize)
				{
					return std::unexpected(ArgError{.arg = std::string{arg},
													.reason = "expected size=WIDTH,HEIGHT with both above zero"});
				}
			}
		}
	}

	return launchOptions;
}
