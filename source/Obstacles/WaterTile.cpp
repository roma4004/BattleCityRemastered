#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/EventSystem.h"

#include <string>

WaterTile::WaterTile(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const int id, const GameMode gameMode)
	: Obstacle{rect,
	           0x1e90ff,
	           1,
	           std::move(window),
	           "Water " + std::to_string(id),
	           std::move(events),
	           id,
	           gameMode}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);
}

void WaterTile::SendDamageStatistics(const std::string& /*author*/, const std::string& /*fraction*/) {}
