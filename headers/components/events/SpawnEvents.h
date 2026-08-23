#pragma once

#include "geometry/Point.h"
#include "geometry/ObjRectangle.h"
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

struct RespawnTankEvent
{
	TankType type;
	Uuid uuid;
	bool skipDelay;
};

struct RespawnCountChangedToEvent
{
	std::string objectName;
	unsigned short respawnCount;
};

struct TankRespawnedEvent
{
	TankType type;
	Uuid uuid;
	FPoint pos;
};

struct BonusSpawnedEvent
{
	FPoint pos;
	BonusType type;
	Uuid uuid;
};

struct ObstacleSpawnedEvent
{
	FPoint pos;
	ObstacleType type;
	Uuid uuid;
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

//NOTE: emitted by every fortress wall the spawner builds - the first one claims the spot, later ones
//just tell FortressManager what stands in it now. The manager keeps the rect after the wall is gone.
struct FortressSpotRegisteredEvent
{
	ObjRectangle rect;
	std::weak_ptr<BaseObj> wall;
};

//NOTE: material is Brick or Steel - the spawner knows no other kind of wall, the fortress framing is
//the caller's
struct SpawnFortressWallEvent
{
	ObjRectangle rect;
	ObstacleType material;
};
