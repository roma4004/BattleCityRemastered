#pragma once

#include "enums/Author.h"
#include "enums/DespawnReason.h"
#include "utils/Uuid.h"
#include <string>


struct TankSpawnEvent
{
	Uuid uuid;
};

struct TankDiedEvent
{
	Author who{};
	Uuid uuid{};
	Author author{};
};

struct DespawnedEvent
{
	std::string who;
	Uuid uuid;
	DespawnReason reason{DespawnReason::None};
};
