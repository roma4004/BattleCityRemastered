#pragma once

#include "../ObjRectangle.h"
#include "../BulletCalibre.h"
#include "utils/Uuid.h"
#include <string>

enum class Direction : char8_t;

struct BulletResetProperty
{
	ObjRectangle rect{};
	Direction dir{};
	int health{};
	std::string author{};
	std::string fraction{};
	Uuid uuid{};
	BulletCalibre calibre{};
};
