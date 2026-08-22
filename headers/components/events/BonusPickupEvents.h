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

struct BonusShovelPickupEvent
{
	std::string fraction;
	std::chrono::milliseconds effectDuration;
};

struct BonusTimerPickupEvent
{
	std::string fraction;
	std::chrono::milliseconds effectDuration;
};

struct BonusStarPickupEvent
{
	std::string author;
	std::string fraction;
};

struct BonusCaliberPickupEvent
{
	std::string author;
	std::string fraction;
};

struct BonusGrenadePickupEvent
{
	std::string author;
	std::string fraction;
};

struct BonusTankPickupEvent
{
	std::string author;
	std::string fraction;
};

struct BonusTimerStatusChangeEvent
{
	std::string fraction;
	bool isActive;
};

struct BonusHelmetStatusChangeEvent
{
	std::string name;
	bool isActive;
};

struct BonusShovelStatusChangeEvent
{
	std::string fraction;
	bool isActive;
};

struct BonusHelmetAppliedEvent
{
	std::string name;
	bool isActive;
};

struct BonusStarAppliedEvent
{
	std::string name;
};

struct BonusCaliberAppliedEvent
{
	std::string name;
};

struct BonusTankAppliedEvent
{
	std::string name;
};
