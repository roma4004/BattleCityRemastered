#pragma once

#include "enums/DespawnReason.h"
#include "utils/Uuid.h"
#include <cstdint>
#include <string>

enum class FortressState : std::uint8_t;

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

struct FortressChangedEvent
{
	FortressState state;
	Uuid uuid;
};
