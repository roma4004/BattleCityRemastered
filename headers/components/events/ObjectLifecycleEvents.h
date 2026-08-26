#pragma once

#include "enums/DespawnReason.h"
#include "utils/Uuid.h"
#include <string>

struct TankSpawnEvent
{
	Uuid uuid;
};

struct TankDiedEvent
{
	Uuid uuid;
};

struct DespawnedEvent
{
	std::string who;
	Uuid uuid;
	DespawnReason reason{DespawnReason::None};
};
