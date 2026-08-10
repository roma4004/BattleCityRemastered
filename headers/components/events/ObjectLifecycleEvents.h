#pragma once

#include <boost/uuid/uuid.hpp>

using buuid = boost::uuids::uuid;

struct TankSpawnEvent
{
	buuid uuid;
};

struct TankDiedEvent
{
	buuid uuid;
};

struct ServerOutDisposeEvent
{
	buuid uuid;
};

struct ClientInDisposeEvent {};
