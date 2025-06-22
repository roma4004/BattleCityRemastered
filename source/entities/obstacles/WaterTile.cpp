#include "entities/obstacles/WaterTile.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

WaterTile::WaterTile(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid,
                     const GameMode gameMode)
	: Obstacle{rect,
	           0x1e90ff,
	           1,
	           "Water",
	           std::move(events),
	           uuid,
	           gameMode,
	           Water}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);
}

WaterTile::~WaterTile() = default;
