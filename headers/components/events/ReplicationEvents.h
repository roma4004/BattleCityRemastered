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

struct TankSpawnCompletedEvent
{
	Uuid uuid;
};
