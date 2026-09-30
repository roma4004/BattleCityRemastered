#pragma once

#include "geometry/Point.h"

struct BulletCaliber
{
	double speed{};
	unsigned int damage{};
	double damageRadius{};
	unsigned short tier{};
	FPoint size{};
};
