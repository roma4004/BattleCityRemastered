#pragma once

#include "utils/Uuid.h"

struct TankSpawnEvent
{
	Uuid uuid;
};

struct TankDiedEvent
{
	Uuid uuid;
};

struct ServerOutDisposeEvent
{
	Uuid uuid;
};

struct ClientInDisposeEvent {};
