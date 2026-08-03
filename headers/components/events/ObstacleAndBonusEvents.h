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


struct FortressChangeEvent
{
	std::string state;
	boost::uuids::uuid uuid;
};

struct BonusSpawnEvent
{
	FPoint pos;
	BonusType type;
	boost::uuids::uuid uuid;
};

struct ObstacleSpawnEvent
{
	ObjRectangle rect;
	ObstacleType type;
	boost::uuids::uuid uuid;
};
