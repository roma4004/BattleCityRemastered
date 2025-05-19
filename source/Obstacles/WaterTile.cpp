#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

WaterTile::WaterTile(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const boost::uuids::uuid uuid, const GameMode gameMode)
	: Obstacle{rect,
	           0x1e90ff,
	           1,
	           std::move(window),
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

void WaterTile::SendDamageStatistics(const std::string& /*author*/, const std::string& /*fraction*/) {}
