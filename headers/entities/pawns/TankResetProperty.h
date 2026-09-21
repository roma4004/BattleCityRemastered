#pragma once

#include "enums/Direction.h"
#include "enums/TankType.h"
#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"

//NOTE: what a pooled tank takes on for its next life - seat and faction follow from the type, the tier is
//given, since a saved match puts a tank back as it was and not as it started
struct TankResetProperty final
{
	Uuid uuid{};
	ObjRectangle rect{};
	int health{};
	double speed{};
	TankType type{};
	Direction dir{};
	unsigned short tier{1u};
};
