#pragma once

#include "geometry/Point.h"
#include "geometry/ObjRectangle.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/ObstacleType.h"
#include "enums/RespawnGroup.h"
#include "enums/TankType.h"
#include "utils/Uuid.h"
#include <chrono>
#include <memory>

class BaseObj;
class Bonus;

struct AddToSpawnQueueEvent
{
	std::shared_ptr<BaseObj> obj;
};

struct BonusCreatedEvent
{
	std::weak_ptr<Bonus> bonus;
	//NOTE: laid out by the map, so no life timer is started for it
	bool isPermanent{};
};

struct RespawnTankEvent
{
	TankType type;
	Uuid uuid;
};

struct RespawnCountChangedToEvent
{
	RespawnGroup group{};
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
	bool isSuper{};
};

struct ObstacleSpawnedEvent
{
	FPoint pos;
	ObstacleType type;
	Uuid uuid;
};

struct SpawnAnimationFinishedEvent
{
	Uuid uuid;
};

struct SpawnObstacleEvent
{
	ObjRectangle rect;
	ObstacleType type;
};

struct SpawnMapBonusEvent
{
	ObjRectangle rect;
	BonusType type;
};

struct BonusReApplyEvent
{
	Uuid uuid;
	Author author{};
};

struct BonusTimerReApplyOnSpawnEvent
{
	bool isEnabled;
};

//NOTE: one per fortress wall built - the first claims the spot, later ones replace its wall; the spot outlives it
struct FortressSpotRegisteredEvent
{
	ObjRectangle rect;
	std::weak_ptr<BaseObj> wall;
};

//NOTE: Brick or Steel only; where the walls frame the fortress is the caller's decision
struct SpawnFortressWallEvent
{
	ObjRectangle rect;
	ObstacleType material;
};
