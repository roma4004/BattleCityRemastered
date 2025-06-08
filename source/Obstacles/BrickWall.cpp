#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

struct Window;

BrickWall::BrickWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const buuid uuid, const GameMode gameMode, std::shared_ptr<IDrawable> textureManager)
	: Obstacle{std::move(rect),
	           0x924b00,
	           1,
	           std::move(window),
	           {"BrickWall"},
	           std::move(events),
	           uuid,
	           gameMode,
	           Brick,
	           std::move(textureManager)}
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
		_events->EmitEvent<const std::string&, const std::string&>("Statistics_BrickWallDied", author, fraction);

		//TODO: move this to onHealthChange
		//for replication
		_events->EmitEvent<const std::string&, const int, const buuid&>("ServerSend_Health", _name, GetHealth(), _uuid);
	}
}
