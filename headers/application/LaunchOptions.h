#pragma once

#include "geometry/Point.h"
#include "enums/GameMode.h"
#include <optional>

//NOTE: empty optional means "not passed" - the ini value stays
struct LaunchOptions final
{
	//NOTE: no mode on the command line means the attract match - the mode only names who fills the
	//seats, the demo itself is a phase
	GameMode gameMode{GameMode::CoopWithBot};
	bool isDemo{true};
	bool skipIntroMusic{false};
	std::optional<UPoint> windowPos{};
	std::optional<UPoint> windowSize{};
};
