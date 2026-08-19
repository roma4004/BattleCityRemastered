#pragma once

#include "geometry/Point.h"

struct BulletCalibre
{
	float speed{};
	unsigned int damage{};
	float damageRadius{};
	unsigned short tier{};
	FPoint size{};
};
