#pragma once

#include "geometry/Point.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"

struct PosChangedEvent
{
	FPoint pos;
	Direction dir;
	Uuid uuid;
};

struct TankShotEvent
{
	Author who{};
	Direction dir;
	Uuid bulletUuid;
};

struct HealthChangedEvent
{
	int health;
	Uuid uuid;
};

//NOTE: the result of an upgrade, not its cause - the client sets it instead of replaying the formula
struct TierChangedEvent
{
	unsigned short tier;
	Uuid uuid;
};

struct TankSpawnCompletedEvent
{
	Uuid uuid;
};

//NOTE: the host sends it only for a bonus that really settled - one picked up mid-burst gets none
struct BonusSpawnCompletedEvent
{
	Uuid uuid;
};
