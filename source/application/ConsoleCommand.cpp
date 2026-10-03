#include "application/ConsoleCommand.h"
#include "enums/PlayerSlot.h"
#include "utils/Log.h"
#include <algorithm>
#include <array>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
constexpr std::string_view kBlanks{" \t\r\n"};

constexpr std::array<std::pair<std::string_view, ConsoleCommand>, 9> kBareCommands{{
		{"/exit", ExitCommand{}},
		{"/restart", RestartCommand{}},
		{"/status", StatusCommand{}},
		{"/players", PlayersCommand{}},
		{"/help", HelpCommand{}},
		{"/close", AcceptClientsCommand{.isAccepting = false}},
		{"/open", AcceptClientsCommand{.isAccepting = true}},
		{"/pause", PauseCommand{.isPaused = true}},
		{"/resume", PauseCommand{.isPaused = false}},
}};

//NOTE: any blanks between words - a pasted line with tabs or a trailing \r reads the same as a typed one
std::vector<std::string_view> Words(const std::string_view line)
{
	std::vector<std::string_view> words;
	for (auto start{line.find_first_not_of(kBlanks)}; start != std::string_view::npos;
		 start = line.find_first_not_of(kBlanks, start))
	{
		const auto end{std::min(line.find_first_of(kBlanks, start), line.size())};
		words.push_back(line.substr(start, end - start));
		start = end;
	}

	return words;
}

std::unexpected<std::string> Rejected(std::string reason) { return std::unexpected{std::move(reason)}; }
}//namespace

std::string MapPathForName(const std::string_view name)
{
	return "Resources/Maps/" + std::string{name} + ".map";
}

std::expected<ConsoleCommand, std::string> ParseConsoleCommand(const std::string_view line)
{
	const std::vector<std::string_view> words{Words(line)};
	if (words.empty() || words.size() > 2u)
	{
		return Rejected("expected a command and at most one argument, /help lists them");
	}

	const std::string_view name{words.front()};
	const std::optional<std::string_view> argument{words.size() == 2u ? std::optional{words.back()} : std::nullopt};

	if (name == "/kick")
	{
		const auto isNamed = [&argument](const PlayerSlot slot) { return argument == ToString(slot); };
		if (const auto slot{std::ranges::find_if(kSlots, isNamed)}; slot != kSlots.end())
		{
			return KickCommand{.slot = *slot};
		}

		return Rejected("expected /kick and a seat, p1 to p4");
	}

	if (name == "/map")
	{
		//NOTE: a name, not a path - a slash or a suffix here would let the console reach outside the maps
		//folder, and the answer to "which levels are there" is the folder listing, not a typed path
		if (!argument || argument->empty()
			|| argument->find_first_of("/\\.") != std::string_view::npos)
		{
			return Rejected("expected /map NAME, where NAME is a file in Resources/Maps without .map");
		}

		return MapCommand{.name = std::string{*argument}};
	}

	if (name == "/log")
	{
		if (argument == "quiet")
		{
			return LogLevelCommand{.level = Log::Level::Quiet};
		}

		if (argument == "normal")
		{
			return LogLevelCommand{.level = Log::Level::Normal};
		}

		if (argument == "detailed")
		{
			return LogLevelCommand{.level = Log::Level::Detailed};
		}

		return Rejected("expected /log quiet, /log normal or /log detailed");
	}

	const auto bare{std::ranges::find(kBareCommands, name, &std::pair<std::string_view, ConsoleCommand>::first)};
	if (bare == kBareCommands.end())
	{
		return Rejected("unknown command " + std::string{name} + ", /help lists them");
	}

	if (argument)
	{
		return Rejected(std::string{name} + " takes no argument");
	}

	return bare->second;
}
