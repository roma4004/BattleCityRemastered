#pragma once

#include "geometry/Point.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"
#include <string>

struct ServerOutPosEvent
{
	std::string who;
	FPoint pos;
	Direction dir;
	Uuid uuid;
};

struct ServerOutShotEvent
{
	std::string who;
	Direction dir;
	Uuid bulletUuid;
};

struct ServerOutHealthEvent
{
	std::string who;
	int health;
	Uuid uuid;
};

// Host -> client: "materialize the tank with this uuid now".
struct ServerOutTankSpawnCompleteEvent
{
	Uuid uuid;
};

struct ServerOutBonusHelmetPickupEvent
{
	std::string name;
	bool isActive;
};

struct ClientInPosEvent
{
	FPoint pos;
	Direction dir;
};

struct ClientInShotEvent
{
	Direction dir;
	Uuid bulletUuid;
};

// Broadcast: no Tank exists yet to key against. TankSpawner looks up uuid in its pending-spawn stash.
struct ClientInTankSpawnCompleteEvent
{
	Uuid uuid;
};

struct ClientInBonusHelmetPickupEvent
{
	bool isEnable;
};

struct ClientInHealthEvent
{
	int health;
};
