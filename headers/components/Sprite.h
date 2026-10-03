#pragma once

#include "enums/Direction.h"
#include "enums/DrawLayer.h"
#include "enums/TextureType.h"
#include "geometry/ObjRectangle.h"

//NOTE: how an object looks this frame - read by the scene painter, drawn by the texture manager
struct Sprite final
{
	DrawLayer layer{};
	ObjRectangle rect{};
	Direction dir{};
	TextureType texture{};
	//NOTE: 0 keeps the atlas colors
	unsigned int rimColor{};
};
