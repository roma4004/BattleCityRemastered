#pragma once

#include "geometry/Point.h"
#include "enums/GameMode.h"
#include "enums/WindowSide.h"
#include <cstdint>
#include <optional>
#include <string>

//NOTE: empty optional means "not passed" - the ini value stays
struct LaunchOptions final
{
	//NOTE: no mode given means the demo; the mode still names who fills the seats, the demo is a phase
	GameMode gameMode{GameMode::CoopWithBot};
	bool isDemo{true};
	bool isMuted{false};
	bool isHelpRequested{false};
	std::optional<UPoint> windowPos{};
	std::optional<UPoint> windowSize{};
	std::optional<WindowSide> windowSide{};
	std::optional<std::string> serverHost{};
	//NOTE: zero is what bind reads as "any free one" - what --port=auto asks for
	std::optional<std::uint16_t> serverPort{};
	//NOTE: server only - where to write the port it actually got, for whoever spawned it
	std::optional<std::string> portFilePath{};
};
