#pragma once

#include "enums/Direction.h"
#include "enums/TankModel.h"
#include "enums/TankType.h"
#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"

//NOTE: a pooled tank's next life - seat, speed and gun follow from type, model and tier; a saved match gives the tier
struct TankResetProperty final
{
	Uuid uuid{};
	ObjRectangle rect{};
	int health{};
	TankType type{};
	TankModel model{};
	Direction dir{};
	unsigned short tier{1u};
};
