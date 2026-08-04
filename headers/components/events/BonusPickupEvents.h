#pragma once

#include <chrono>
#include <string>

struct BonusHelmetPickupEvent
{
	std::string author;
	std::chrono::milliseconds effectDuration;
};

struct BonusTeamEffectPickupEvent
{
	std::string fraction;
	std::chrono::milliseconds effectDuration;
};
