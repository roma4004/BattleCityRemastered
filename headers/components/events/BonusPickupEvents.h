#pragma once

#include <string>

enum class Faction : char8_t;

//NOTE: a pickup is keyed by the side its effect lands on - the tank that picked it up, or a faction
//when the whole team is hit. The key already says who, so there is nothing left to put inside
struct BonusStarPickupEvent {};

struct BonusCaliberPickupEvent {};

struct BonusShipPickupEvent {};

struct BonusGrenadePickupEvent {};

struct BonusHelmetPickupEvent
{
	std::string author;
};

struct BonusTankPickupEvent
{
	std::string author;
};

//NOTE: BonusManager owns both teams' effects at once, so the faction is payload here too
struct BonusShovelPickupEvent
{
	Faction faction{};
};

//NOTE: whom to freeze, not who picked it up - the bonus itself decides that it hits the other side
struct BonusTimerPickupEvent
{
	Faction target{};
};

//NOTE: keyed by the faction it is on
struct BonusTimerStatusChangeEvent
{
	bool isActive;
};

//NOTE: keyed by the tank it is on
struct BonusHelmetStatusChangeEvent
{
	bool isActive;
};

struct BonusShovelStatusChangeEvent
{
	Faction faction{};
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

struct BonusShipAppliedEvent
{
	std::string name;
};

struct BonusTankAppliedEvent
{
	std::string name;
};
