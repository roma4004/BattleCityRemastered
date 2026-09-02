#pragma once

#include "enums/Author.h"

struct BrickWallDiedEvent final
{
	Author author{};
};

struct SteelWallDiedEvent final
{
	Author author{};
};

struct StatisticsBulletHitEvent final
{
	Author author{};
};

struct StatisticsBonusPickupEvent final
{
	Author author{};
};

struct StatisticsBonusDestroyedEvent final
{
	Author author{};
};

//NOTE: bonus just ran out its timeout
struct StatisticsBonusExpiredEvent final {};

struct StatisticsTankHitEvent final
{
	//NOTE: the seat that was hit and the seat that hit it
	Author who{};
	Author author{};
};
