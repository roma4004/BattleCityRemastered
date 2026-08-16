#pragma once

#include "Point.h"
#include "enums/GameMode.h"
#include <optional>

//NOTE: empty optional means "not passed" - the ini value stays
struct LaunchOptions final
{
	GameMode gameMode{GameMode::Demo};
	bool skipIntroMusic{false};
	std::optional<UPoint> windowPos{};
	std::optional<UPoint> windowSize{};
};
