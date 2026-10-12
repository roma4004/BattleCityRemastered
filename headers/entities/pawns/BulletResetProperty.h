#pragma once

#include "enums/Author.h"
#include "geometry/ObjRectangle.h"
#include "../BulletCaliber.h"
#include "utils/Uuid.h"

enum class Direction : char8_t;

struct BulletResetProperty
{
	ObjRectangle rect{};
	Direction dir{};
	unsigned int health{};
	Author author{};
	Uuid authorUuid{};
	BulletCaliber caliber{};
};
