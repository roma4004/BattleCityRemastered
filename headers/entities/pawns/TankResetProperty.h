#pragma once

#include "enums/Author.h"
#include "enums/Direction.h"
#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"

//NOTE: everything a pooled tank has to forget between two lives - the faction and the tier follow
//from the seat, so they are not fields here
struct TankResetProperty final
{
	Uuid uuid{};
	ObjRectangle rect{};
	int health{};
	double speed{};
	Author author{};
	Direction dir{};
};
