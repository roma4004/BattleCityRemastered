#include "../../headers/obstacles/GrassTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

GrassTile::GrassTile(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode)
	: Obstacle{rect,
	           0x1e90ff,
	           1,
	           "Grass",
	           std::move(events),
	           uuid,
	           gameMode,
	           Grass}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);
}

GrassTile::~GrassTile() = default;
