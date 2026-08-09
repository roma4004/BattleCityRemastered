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

class BaseObj;

using buuid = boost::uuids::uuid;

struct AddToSpawnQueueEvent
{
	std::shared_ptr<BaseObj> obj;
};

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

struct ClientInRespawnTankEvent
{
	TankType type;
	buuid uuid;
	ObjRectangle rect;
};

struct RespawnTankEvent
{
	TankType type;
	buuid uuid;
	bool skipDelay;
};

struct ClientInBonusSpawnEvent
{
	FPoint pos;
	BonusType type;
	buuid uuid;
};

struct ClientInObstacleSpawnEvent
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

struct ServerOutRespawnTankEvent
{
	TankType type;
	buuid uuid;
	ObjRectangle rect;
};

struct SpawnDelayStartEvent
{
	buuid uuid;
	std::chrono::milliseconds delay;
};

struct SpawnObstacleEvent
{
	ObjRectangle rect;
	ObstacleType type;
};

struct BonusEffectReApplyEvent
{
	buuid uuid;
	std::string name;
	std::string fraction;
};

struct BonusTimerReApplyOnSpawnEvent
{
	bool isEnabled;
};
