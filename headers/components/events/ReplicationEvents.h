#pragma once

#include "geometry/Point.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"
#include <string>

struct PosChangedEvent
{
	std::string who;
	FPoint pos;
	Direction dir;
	Uuid uuid;
};

struct TankShotEvent
{
	std::string who;
	Direction dir;
	Uuid bulletUuid;
};

struct HealthChangedEvent
{
	std::string who;
	int health;
	Uuid uuid;
};

//NOTE: the result of an upgrade, not its cause - the client sets it instead of replaying the formula
struct TierChangedEvent
{
	std::string who;
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
