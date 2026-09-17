#pragma once

#include "enums/Author.h"

enum class Faction : char8_t;

//NOTE: keyed by whom the effect lands on - the picking tank's seat, or a faction for a whole team - so no payload
struct BonusStarPickupEvent {};

struct BonusCaliberPickupEvent {};

struct BonusShipPickupEvent {};

struct BonusGrenadePickupEvent {};

struct BonusHelmetPickupEvent
{
	Author author{};
};

struct BonusTankPickupEvent
{
	Author author{};
};

//NOTE: broadcast - BonusManager handles both teams' effects, so the faction travels as payload
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

//NOTE: keyed by the seat it is on
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
	Author author{};
	bool isActive;
};

struct BonusShipAppliedEvent
{
	Author author{};
};

struct BonusTankAppliedEvent
{
	Author author{};
};
