#include "entities/obstacles/EagleTile.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

EagleTile::EagleTile(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid,
                     const GameMode gameMode)
	: Obstacle{rect,
	           0x1e90ff,
	           1,
	           "Eagle",
	           std::move(events),
	           uuid,
	           gameMode,
	           ObstacleType::Eagle}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
}

EagleTile::~EagleTile() = default;
