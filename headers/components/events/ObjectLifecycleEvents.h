#pragma once

#include "enums/DespawnReason.h"
#include "utils/Uuid.h"
#include <string>

enum class Faction : char8_t;

struct TankSpawnEvent
{
	Uuid uuid;
};

struct TankDiedEvent
{
	std::string who{};
	Uuid uuid{};
	std::string author{};
	Faction faction{};
};

struct DespawnedEvent
{
	std::string who;
	Uuid uuid;
	DespawnReason reason{DespawnReason::None};
};
