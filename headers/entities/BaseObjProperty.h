#pragma once

#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"
#include <string>

struct BaseObjProperty
{
	ObjRectangle rect{};
	int health{0};
	Uuid uuid{};
	std::string name{};
	std::string fraction{};
	bool isAlive{true};
};
