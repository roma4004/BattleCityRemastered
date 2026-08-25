#pragma once

#include "geometry/ObjRectangle.h"
#include "../BulletCalibre.h"
#include "utils/Uuid.h"
#include <string>

enum class Faction : char8_t;
enum class Direction : char8_t;

struct BulletResetProperty
{
	ObjRectangle rect{};
	Direction dir{};
	int health{};
	std::string author{};
	Faction faction{};
	Uuid uuid{};
	BulletCalibre calibre{};
};
