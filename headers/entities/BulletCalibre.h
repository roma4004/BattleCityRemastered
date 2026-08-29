#pragma once

#include "geometry/Point.h"

struct BulletCalibre
{
	double speed{};
	unsigned int damage{};
	double damageRadius{};
	unsigned short tier{};
	FPoint size{};
};
