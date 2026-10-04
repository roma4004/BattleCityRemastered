#pragma once

#include "LaunchOptions.h"
#include <array>
#include <expected>
#include <string>

struct ArgError final
{
	std::string arg{};
	std::string reason{};
};

//NOTE: what --help prints, and the only list of what Parse accepts - a new option is added in both
inline constexpr std::array kUsage{"--server               bring a server up and play on it",
								   "--client               join a match",
								   "--ffa                  free for all, alone against the bots",
								   "--2p-ffa               free for all, two at one keyboard and two bots at a time",
								   "--address=IP           IPv4 or IPv6; left out, this machine's own on its network",
								   "--port=N               the port; left out, or auto, takes any free one",
								   "--size=WIDTH,HEIGHT    window size, unscaled",
								   "--pos=X,Y              window position",
								   "--side=left|right      which half of the screen the window takes",
								   "--mute                 start with the sound off",
								   "--help                 this text"};

//NOTE: the server has no window and no seat, so it takes neither - the same list for ParseServer
inline constexpr std::array kServerUsage{"--address=IP           what to listen on - 0.0.0.0 is every interface; "
										 "left out, this machine's own on its network",
										 "--port=N               the port to listen on; left out, or auto, takes any free one",
										 "--port-file=PATH       write the port it got here, for whoever spawned it",
										 "--seats=N              how many players the match takes, 1 to 4; left out, 2",
										 "--rules=classic|ffa    classic or free for all; left out, classic",
										 "--map=NAME             first map, a file in Resources/Maps without .map",
										 "--enemies=N            enemies at once in classic, 1 to 4; left out, 4",
										 "--bots=N               bots in the seats nobody took, up to one fewer than "
										 "the seats; left out, 0",
										 "--start=now|full       now starts with whoever is in, full waits for every "
										 "seat; left out, full",
										 "--help                 this text"};

class CommandLineParser final
{
public:
	//NOTE: an unknown argument or a malformed value fails the whole parse, so a typo never passes for a dead flag
	[[nodiscard]] static std::expected<LaunchOptions, ArgError> Parse(int argc, const char* const* argv);

	//NOTE: a game option reaching the server would be taken and dropped on the floor, so it is an error here
	[[nodiscard]] static std::expected<LaunchOptions, ArgError> ParseServer(int argc, const char* const* argv);
};
