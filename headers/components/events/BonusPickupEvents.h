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

//NOTE: keyed the same; charges are the hits it takes
struct BonusShipPickupEvent
{
	unsigned short charges{};
};

//NOTE: keyed by the taker's faction
struct BonusTeamShipPickupEvent
{
	unsigned short charges{};
};

inline constexpr unsigned short kGoldenShipCharges{2u};

//NOTE: spares its taker, who in a free-for-all is on the side it hits
struct BonusGrenadePickupEvent
{
	Author spared{};
};

//NOTE: sent before the blast, so the deaths see the reserve already cut
struct BonusGoldenGrenadePickupEvent
{
	Author taker{};
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
	bool isSuper{};
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

//NOTE: out of charges, still active while afloat
struct BonusShipAppliedEvent
{
	Author author{};
	bool isActive{};
	unsigned short charges{};
};


