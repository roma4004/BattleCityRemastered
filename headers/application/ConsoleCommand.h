#pragma once

#include "enums/PlayerSlot.h"
#include "utils/Log.h"
#include <array>
#include <expected>
#include <string>
#include <string_view>
#include <variant>

struct ExitCommand {};
struct RestartCommand {};
struct StatusCommand {};
struct PlayersCommand {};
struct HelpCommand {};

struct KickCommand
{
	PlayerSlot slot;
};

//NOTE: /close and /open - a client already seated stays either way
struct AcceptClientsCommand
{
	bool isAccepting;
};

struct PauseCommand
{
	bool isPaused;
};

struct LogLevelCommand
{
	Log::Level level;
};

using ConsoleCommand = std::variant<ExitCommand, RestartCommand, StatusCommand, PlayersCommand, HelpCommand,
									KickCommand, AcceptClientsCommand, PauseCommand, LogLevelCommand>;

inline constexpr std::array kConsoleHelp{
		"/exit - stop the server",
		"/restart - restart the match",
		"/status - phase, uptime, fps, port, seats",
		"/players - who sits where, from which address, for how long",
		"/kick p1|p2 - send a seat's client away for good",
		"/close, /open - stop and resume taking new clients",
		"/pause, /resume - pause and resume the match",
		"/log quiet|normal|detailed - how much the log says",
		"/help - this list"};

[[nodiscard]] std::expected<ConsoleCommand, std::string> ParseConsoleCommand(std::string_view line);
