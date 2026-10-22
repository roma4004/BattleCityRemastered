#pragma once

#include "entities/EffectTarget.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include <chrono>
#include <vector>

enum class Faction : char8_t;

//NOTE: keyed by whom the effect lands on - the picking tank's seat, or a faction for a whole team - so no payload
struct BonusStarPickupEvent {};

struct BonusCaliberPickupEvent {};

struct BonusShipPickupEvent {};

//NOTE: spares its taker, who in a free-for-all is on the side it hits
struct BonusGrenadePickupEvent
{
	Author spared{};
};

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
	Author spared{};
};

//NOTE: keyed by the frozen faction; in a free-for-all the taker is not frozen
struct BonusTimerStatusChangeEvent
{
	bool isActive;
	Author spared{};
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

struct BonusEffectTimeLeft
{
	BonusType type{};
	EffectTarget target{};
	Author spared{};
	std::chrono::milliseconds duration{};
	std::chrono::milliseconds left{};
};

//NOTE: the whole running set, sent on each change
struct BonusEffectsChangedEvent
{
	std::vector<BonusEffectTimeLeft> effects{};
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
