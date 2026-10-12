#pragma once

#include "geometry/Point.h"

struct BulletCaliber
{
	double speed{};
	unsigned int damage{};
	//NOTE: the shell's own, apart from its damage - spent penetrating walls and on a shell it meets
	unsigned int health{};
	double damageRadius{};
	unsigned short tier{};
	bool penetratesWalls{};
	FPoint size{};
};
