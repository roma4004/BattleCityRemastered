#pragma once

#include "ObjRectangle.h"

#include <string>

struct BaseObjProperty
{
	ObjRectangle rect{};
	int color{0};
	int health{0};
	bool isAlive{true};
	int id{0};
	std::string name{};
	std::string fraction{};
};
