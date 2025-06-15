#include "../../headers/obstacles/EagleTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

EagleTile::EagleTile(ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode)
	: Obstacle{std::move(rect),
	           0x1e90ff,
	           1,
	           "Eagle",
	           std::move(events),
	           uuid,
	           gameMode,
	           Eagle}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
}

EagleTile::~EagleTile() = default;
