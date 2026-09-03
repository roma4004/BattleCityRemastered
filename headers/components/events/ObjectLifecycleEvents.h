#pragma once

#include "enums/Author.h"
#include "enums/DespawnReason.h"
#include "utils/Uuid.h"


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
	Uuid uuid;
	DespawnReason reason{DespawnReason::None};
};
