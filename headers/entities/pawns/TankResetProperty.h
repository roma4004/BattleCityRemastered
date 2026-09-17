#pragma once

#include "enums/Author.h"
#include "enums/Direction.h"
#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"

//NOTE: what a pooled tank takes on for its next life; the faction follows from the author, the tier restarts at 1
struct TankResetProperty final
{
	Uuid uuid{};
	ObjRectangle rect{};
	int health{};
	double speed{};
	Author author{};
	Direction dir{};
};
