#pragma once

#include "geometry/Point.h"

struct BulletCaliber
{
	double speed{};
	unsigned int damage{};
	//NOTE: the shell's own health, not its damage - spent on wall layers and on the shells it meets
	unsigned int health{};
	double blastRadius{};
	unsigned short tier{};
	FPoint size{};
};
