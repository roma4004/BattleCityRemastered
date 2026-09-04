#pragma once

#include "enums/Author.h"
#include "geometry/Point.h"
#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"
#include <string>

enum class Direction : char8_t;
enum class AnimationType : char8_t;
enum class TextureType : char8_t;

//NOTE: these two are also the spawn's clock - AnimationManager reports the last frame by uuid
struct AnimationCreateTankSpawnEvent
{
	ObjRectangle rect;
	Uuid uuid;
};

//NOTE: the burst is the countdown, so a cancelled spawn has to stop it or the tank still lands
struct AnimationCancelTankSpawnEvent
{
	Uuid uuid;
};

struct AnimationCreateBonusSpawnEvent
{
	ObjRectangle rect;
	Uuid uuid;
};

struct AnimationTankUpdateEvent
{
	Author author;
	FPoint pos;
	Direction dir;
};

struct AnimationCreateTankExplosionEvent
{
	ObjRectangle rect;
	Author author;
};

struct AnimationCreateBulletExplosionEvent
{
	ObjRectangle rect;
};

struct AnimationCreateTankMoveEvent
{
	ObjRectangle rect;
	Author author;
};

struct AnimationBonusHelmetChangeEvent
{
	Author author;
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
	Author author;
};

struct DrawObjEvent
{
	ObjRectangle rect;
	Direction dir;
	TextureType texture;
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
