#pragma once

#include "geometry/Point.h"
#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"
#include <string>

enum class Direction : char8_t;
enum class AnimationType : char8_t;

//NOTE: these two are also the spawn's clock - AnimationManager reports the last frame by uuid
struct AnimationCreateTankSpawnEvent
{
	ObjRectangle rect;
	std::string name;
	Uuid uuid;
};

struct AnimationCreateBonusSpawnEvent
{
	ObjRectangle rect;
	Uuid uuid;
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
	AnimationType type;
	//NOTE: only the per-tank animations need it - the rest are told apart by the type alone
	std::string name;
};

struct DrawObjEvent
{
	ObjRectangle rect;
	Direction dir;
	std::string name;
	unsigned int rimColor{};//NOTE: 0 leaves the sprite frame as the atlas has it
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
