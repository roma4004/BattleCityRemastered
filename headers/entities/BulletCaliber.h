#pragma once

#include "geometry/Point.h"

struct BulletCaliber
{
	double speed{};
	unsigned int damage{};
	//NOTE: the shell's own, apart from its damage - spent sinking into walls and on a shell it meets
	int health{};
	double damageRadius{};
	unsigned short tier{};
	FPoint size{};
};
