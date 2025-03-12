#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/EventSystem.h"

#include <string>

SteelWall::SteelWall(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const int id, const GameMode gameMode)
	: Obstacle{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1},
	           0xaaaaaa,
	           1,
	           std::move(window),
	           "SteelWall " + std::to_string(id),//TODO: change name for statistics
	           std::move(events),
	           id,
	           gameMode}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);
}

void SteelWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("SteelWallDied", author, fraction);
	}
}
