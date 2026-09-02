#pragma once

#include "enums/Author.h"
#include "geometry/ObjRectangle.h"
#include "../BulletCalibre.h"
#include "utils/Uuid.h"

enum class Direction : char8_t;

struct BulletResetProperty
{
	ObjRectangle rect{};
	Direction dir{};
	int health{};
	Author author{};
	Uuid authorUuid{};
	BulletCalibre calibre{};
};
