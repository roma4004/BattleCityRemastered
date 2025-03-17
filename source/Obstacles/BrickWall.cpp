#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/EventSystem.h"

#include <string>

struct Window;

BrickWall::BrickWall(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const int id, const GameMode gameMode)
	: Obstacle{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1},
	           0x924b00,
	           1,
	           std::move(window),
	           {"BrickWall" + std::to_string(id)},//TODO: change name for statistics
	           std::move(events),
	           id,
	           gameMode}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
}

void BrickWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		//TODO: generalize who died in {_Name}Died and move method to Obstacle
		_events->EmitEvent<const std::string&, const std::string&>("BrickWallDied", author, fraction);//for statistic

		//TODO: move this to onHealthChange
		_events->EmitEvent<const std::string&, const int>("ServerSend_Health", GetName(), GetHealth());//for replication
	}
}
