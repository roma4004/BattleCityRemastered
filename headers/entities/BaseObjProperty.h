#pragma once

#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"
#include <string>

enum class Faction : char8_t;

struct BaseObjProperty
{
	ObjRectangle rect{};
	int health{0};
	Uuid uuid{};
	std::string name{};
	Faction faction{};
	bool isAlive{true};
};
