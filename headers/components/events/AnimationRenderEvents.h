#pragma once

#include "geometry/Point.h"
#include "geometry/ObjRectangle.h"
#include <string>

enum class Direction : char8_t;

struct AnimationCreateTankSpawnEvent
{
	ObjRectangle rect;
	std::string name;
};

struct AnimationTankUpdateEvent
{
	std::string name;
	FPoint pos;
	Direction dir;
};

struct AnimationCreateExplosionEvent
{
	ObjRectangle rect;
	std::string name;
};

struct AnimationCreateTankExplosionEvent
{
	ObjRectangle rect;
	std::string name;
};

struct AnimationCreateBulletExplosionEvent
{
	ObjRectangle rect;
	std::string name;
};

struct AnimationCreateTankMoveEvent
{
	ObjRectangle rect;
	std::string name;
};

struct AnimationBonusHelmetChangeEvent
{
	std::string name;
	bool isEnable;
};

struct DrawAnimationEvent
{
	ObjRectangle rect;
	Direction dir;
	int frame;
	int scale;
	std::string name;
};

struct DrawObjEvent
{
	ObjRectangle rect;
	Direction dir;
	std::string name;
};

struct RenderHealthBarEvent
{
	ObjRectangle rect;
	int health;
};

struct RenderTextEvent
{
	Point pos;
	unsigned int color;
	std::string text;
};

struct RenderTextureEvent
{
	ObjRectangle textureRect;
	ObjRectangle destRect;
	Direction dir;
};

struct AnimationCreateWaterEvent
{
	ObjRectangle rect;
};

struct RenderColorTextureEvent
{
	ObjRectangle rect;
};
