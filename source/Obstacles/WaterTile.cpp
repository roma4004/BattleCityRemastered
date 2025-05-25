#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

WaterTile::WaterTile(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const boost::uuids::uuid uuid, const GameMode gameMode,
                     std::shared_ptr<SDL_Texture> textureCollection,
                     std::shared_ptr<SDL_Renderer> renderer)
	: Obstacle{std::move(rect),
	           0x1e90ff,
	           1,
	           std::move(window),
	           "Water",
	           std::move(events),
	           uuid,
	           gameMode,
	           Water,
	           std::move(textureCollection),
	           std::move(renderer)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);
}

void WaterTile::SendDamageStatistics(const std::string& /*author*/, const std::string& /*fraction*/) {}
