#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include "enums/AnimationType.h"
#include "enums/BonusType.h"
#include "enums/ObstacleType.h"
#include "enums/TankType.h"
#include <boost/uuid/uuid.hpp>
#include <chrono>
#include <memory>
#include <string>

class Tank;

using buuid = boost::uuids::uuid;

struct AnimationCreateEvent
{
	AnimationType type;
	ObjRectangle rect;
	std::string name;
};

struct AnimationCreateTankEvent
{
	ObjRectangle rect;
	std::string name;
};

struct ClientReceivedRespawnTankEvent
{
	TankType type;
	buuid uuid;
	ObjRectangle rect;
};

struct ClientReceivedBonusSpawnEvent
{
	FPoint pos;
	BonusType type;
	buuid uuid;
};

struct ClientReceivedObstacleSpawnEvent
{
	ObjRectangle rect;
	ObstacleType type;
	buuid uuid;
};

struct RespawnCountChangedToEvent
{
	std::string objectName;
	unsigned short respawnCount;
};

struct ServerSendRespawnTankEvent
{
	TankType type;
	buuid uuid;
	ObjRectangle rect;
};

struct SpawnDelayStartEvent
{
	std::shared_ptr<Tank> tank;
	std::chrono::milliseconds delay;
};

struct SpawnObstacleEvent
{
	ObjRectangle rect;
	ObstacleType type;
};
