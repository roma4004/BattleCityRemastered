#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include <string>

enum class Direction : char8_t;
enum class AnimationType : char8_t;

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

struct AnimationTankUpdateEvent
{
	std::string name;
	FPoint pos;
	Direction dir;
};

struct BonusHelmetAnimationChangeEvent
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

struct ServerOutAnimationCreateEvent
{
	AnimationType type;
	ObjRectangle rect;
	std::string name;
};

struct AnimationCreateWaterEvent
{
	ObjRectangle rect;
};

struct RenderColorTextureEvent
{
	ObjRectangle rect;
};
