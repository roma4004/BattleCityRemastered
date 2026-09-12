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

//NOTE: the world has just been swept - what a pool reclaims is unreachable by the time it hears this
struct DeadObjectsSweptEvent {};

struct DespawnedEvent
{
	Uuid uuid;
	DespawnReason reason{DespawnReason::None};
};
