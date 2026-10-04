#pragma once

#include "geometry/Point.h"
#include "enums/GameMode.h"
#include "enums/MatchRules.h"
#include "enums/WindowSide.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

//NOTE: empty optional means "not passed" - the ini value stays
struct LaunchOptions final
{
	//NOTE: no mode given means the demo; the mode still names who fills the seats, the demo is a phase
	GameMode gameMode{GameMode::CoopWithBot};
	bool isDemo{true};
	bool isMuted{};
	bool isHelpRequested{};
	std::optional<UPoint> windowPos{};
	std::optional<UPoint> windowSize{};
	std::optional<WindowSide> windowSide{};
	std::optional<std::string> serverHost{};
	//NOTE: zero is what bind reads as "any free one" - what --port=auto asks for
	std::optional<std::uint16_t> serverPort{};
	//NOTE: server only - where to write the port it actually got, for whoever spawned it
	std::optional<std::string> portFilePath{};
	//NOTE: server only - how many players the match takes
	std::optional<std::size_t> seats{};
	//NOTE: server only - who fights whom
	std::optional<MatchRules> rules{};
	//NOTE: server only - the first match's map, by name
	std::optional<std::string> mapName{};
	//NOTE: server only - how many enemies the field holds at once
	std::optional<std::size_t> enemiesAtOnce{};
	//NOTE: server only - how many of the seats nobody took bots fill
	std::optional<std::size_t> bots{};
	//NOTE: server only - start with whoever is in instead of waiting for every seat
	std::optional<bool> isStartingAtOnce{};
};
