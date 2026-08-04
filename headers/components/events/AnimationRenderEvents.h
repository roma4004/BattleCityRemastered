#pragma once

#include "Point.h"
#include "entities/ObjRectangle.h"
#include <string>

enum class Direction : char8_t;
enum class AnimationType : char8_t;

struct DrawAnimationEvent
{
	ObjRectangle rect;
	Direction dir;
	int frame;
	int scale;
	std::string name;
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

struct ServerSendAnimationCreateEvent
{
	AnimationType type;
	ObjRectangle rect;
	std::string name;
};
