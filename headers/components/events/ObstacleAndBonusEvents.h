#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include "utils/Uuid.h"
#include <cstdint>
#include <string>

enum class BonusType : char8_t;
enum class ObstacleType : char8_t;
enum class FortressState : std::uint8_t;

struct StatisticsAttributionEvent
{
	std::string author;
	std::string fraction;
};


struct ServerOutFortressChangeEvent
{
	FortressState state;
	Uuid uuid;
};

//NOTE: separate types from ServerOutFortressChangeEvent above - NetworkTest puts a ServerHandler and
//a ClientHandler on one EventSystem, where a shared type would loop the host's own emit straight into
//the receiving listener, with no round trip in between
struct ClientInFortressDiedEvent {};

struct ClientInFortressToBrickEvent {};

struct ClientInFortressToSteelEvent {};

struct ServerOutBonusSpawnEvent
{
	FPoint pos;
	BonusType type;
	Uuid uuid;
};

struct ServerOutObstacleSpawnEvent
{
	ObjRectangle rect;
	ObstacleType type;
	Uuid uuid;
};

struct ServerOutBonusDeSpawnEvent
{
	Uuid uuid;
};

struct ClientInBonusDeSpawnEvent
{
	Uuid uuid;
};
