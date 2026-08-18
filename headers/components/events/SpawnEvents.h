#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include "enums/BonusType.h"
#include "enums/ObstacleType.h"
#include "enums/TankType.h"
#include "utils/Uuid.h"
#include <chrono>
#include <memory>
#include <string>

class BaseObj;

struct AddToSpawnQueueEvent
{
	std::shared_ptr<BaseObj> obj;
};

struct ClientInRespawnTankEvent
{
	TankType type;
	Uuid uuid;
	ObjRectangle rect;
};

struct RespawnTankEvent
{
	TankType type;
	Uuid uuid;
	bool skipDelay;
};

struct ClientInBonusSpawnEvent
{
	FPoint pos;
	BonusType type;
	Uuid uuid;
};

struct ClientInObstacleSpawnEvent
{
	ObjRectangle rect;
	ObstacleType type;
	Uuid uuid;
};

struct RespawnCountChangedToEvent
{
	std::string objectName;
	unsigned short respawnCount;
};

struct ServerOutRespawnTankEvent
{
	TankType type;
	Uuid uuid;
	ObjRectangle rect;
};

struct SpawnDelayStartEvent
{
	Uuid uuid;
	std::chrono::milliseconds delay;
};

struct TankSpawnDelayFinishedEvent
{
	Uuid uuid;
};

struct SpawnObstacleEvent
{
	ObjRectangle rect;
	ObstacleType type;
};

struct BonusReApplyEvent
{
	Uuid uuid;
	std::string name;
	std::string fraction;
};

struct BonusTimerReApplyOnSpawnEvent
{
	bool isEnabled;
};
