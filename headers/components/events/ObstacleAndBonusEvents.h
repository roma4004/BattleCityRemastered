#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include <boost/uuid/uuid.hpp>
#include <string>

enum class BonusType : char8_t;
enum class ObstacleType : char8_t;

struct StatisticsAttributionEvent
{
	std::string author;
	std::string fraction;
};


struct ServerOutFortressChangeEvent
{
	std::string state;
	boost::uuids::uuid uuid;
};

//NOTE: kept distinct from ServerOutFortressChangeEvent (used server-trigger-side) rather than
//reused, because in-process code (e.g. UnitTests/NetworkTest.cpp, which constructs both a
//ServerHandler and a ClientHandler on the same EventSystem instance to simulate a round trip)
//would otherwise have a direct emit loop back into its own "received" listener before any real
//network round trip happens. Matches every other replicated pair's pattern, e.g. ServerOutPosEvent
//vs ClientInPosEvent.
struct ClientInFortressChangeEvent
{
	std::string state;
	boost::uuids::uuid uuid;
};

struct ServerOutBonusSpawnEvent
{
	FPoint pos;
	BonusType type;
	boost::uuids::uuid uuid;
};

struct ServerOutObstacleSpawnEvent
{
	ObjRectangle rect;
	ObstacleType type;
	boost::uuids::uuid uuid;
};

struct ServerOutBonusDeSpawnEvent
{
	boost::uuids::uuid uuid;
};

struct ClientInBonusDeSpawnEvent
{
	boost::uuids::uuid uuid;
};
