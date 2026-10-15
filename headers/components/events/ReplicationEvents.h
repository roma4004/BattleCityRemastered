#pragma once

#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "utils/Uuid.h"

struct WorldSnapshot;

struct PosChangedEvent
{
	FPoint pos;
	Direction dir;
	Uuid uuid;
};

struct TankShotEvent
{
	Author who{};
	Direction dir;
	Uuid bulletUuid;
};

struct HealthChangedEvent
{
	unsigned int health;
	Uuid uuid;
};

//NOTE: the tier an upgrade ended at - the client sets it as is
struct TierChangedEvent
{
	unsigned short tier;
	Uuid uuid;
};

struct TankGrewEvent
{
	ObjRectangle rect;
	Uuid uuid;
};

struct TankSpawnCompletedEvent
{
	Uuid uuid;
};

//NOTE: where the burst stands now - the client mirrors the host's square, it shoves nothing itself
struct TankSpawnMovedEvent
{
	Uuid uuid;
	FPoint pos{};
};

//NOTE: the host sends it only for a bonus that really settled - one picked up mid-burst gets none
struct BonusSpawnCompletedEvent
{
	Uuid uuid;
};

//NOTE: filled in place - every holder of a piece of the field writes its own part
struct WorldSnapshotRequestedEvent
{
	WorldSnapshot& snapshot;
};

//NOTE: the mirror has just been reset - every holder rebuilds its own part
struct WorldSnapshotReceivedEvent
{
	const WorldSnapshot& snapshot;
};
