#pragma once

#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"

enum class Direction : char8_t;

//NOTE: also the spawn's clock - AnimationManager reports the last frame by uuid; an endless burst is a
//client's, which lands on the host's word and so plays until TankSpawnCompletedEvent
struct AnimationCreateTankSpawnEvent
{
	ObjRectangle rect;
	Uuid uuid;
	bool isEndless{};
};

//NOTE: a cancelled burst never reports, so the tank waiting for it never lands
struct AnimationCancelTankSpawnEvent
{
	Uuid uuid;
};

//NOTE: the same burst carried to another square - creating a fresh one would restart the countdown
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
