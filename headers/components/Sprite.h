#pragma once

#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/DrawLayer.h"
#include "enums/TankModel.h"
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
	//NOTE: a tank's - the seat picks the color, the model and the tier its shape
	Author author{};
	TankModel model{};
	unsigned short tier{1u};
	//NOTE: the step of a texture that changes - a tank's track, the helmet's shimmer
	int frame{};
};
