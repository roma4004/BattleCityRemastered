#pragma once

#include "Point.h"
#include "enums/Direction.h"
#include <boost/uuid/uuid.hpp>
#include <string>

using buuid = boost::uuids::uuid;

struct ServerOutPosEvent
{
	std::string who;
	FPoint pos;
	Direction dir;
	buuid uuid;
};

struct ServerOutShotEvent
{
	std::string who;
	Direction dir;
	buuid bulletUuid;
};

struct ServerOutHealthEvent
{
	std::string who;
	int health;
	buuid uuid;
};

// Host -> client: "materialize the tank with this uuid now".
struct ServerOutTankSpawnCompleteEvent
{
	buuid uuid;
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
	buuid bulletUuid;
};

// Broadcast: no Tank exists yet to key against. TankSpawner looks up uuid in its pending-spawn stash.
struct ClientInTankSpawnCompleteEvent
{
	buuid uuid;
};

struct ClientInBonusHelmetPickupEvent
{
	bool isEnable;
};

struct ClientInHealthEvent
{
	int health;
};
