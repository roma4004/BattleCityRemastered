#pragma once

#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"

enum class Direction : char8_t;

//NOTE: also the spawn's clock - the last frame is reported by uuid; a client's burst runs until TankSpawnCompletedEvent
struct AnimationCreateTankSpawnEvent
{
	ObjRectangle rect;
	Uuid uuid;
	bool isEndless{};
};

//NOTE: a cancelled burst never reports its last frame, so its tank never spawns
struct AnimationCancelTankSpawnEvent
{
	Uuid uuid;
};

//NOTE: moves the burst to another square; a new burst would restart the countdown
struct AnimationMoveTankSpawnEvent
{
	ObjRectangle rect;
	Uuid uuid;
};

struct AnimationCreateBonusSpawnEvent
{
	ObjRectangle rect;
	Uuid uuid;
	bool isEndless{};
};

struct AnimationCreateTankExplosionEvent
{
	ObjRectangle rect;
};

struct AnimationCreateBulletExplosionEvent
{
	ObjRectangle rect;
};

struct RenderHealthBarEvent
{
	ObjRectangle rect;
	unsigned int health;
};

struct RenderTextureEvent
{
	ObjRectangle textureRect;
	ObjRectangle destRect;
	Direction dir;
	unsigned int color{};//NOTE: 0 draws the sprite untinted
};

struct AnimationCreateWaterEvent
{
	ObjRectangle rect;
};

struct RenderColorTextureEvent
{
	ObjRectangle rect;
};
