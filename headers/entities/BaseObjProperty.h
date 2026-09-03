#pragma once

#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"

enum class Faction : char8_t;

struct BaseObjProperty
{
	ObjRectangle rect{};
	int health{0};
	Uuid uuid{};
	Faction faction{};
	bool isAlive{true};
};
