#include "entities/obstacles/WaterTile.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

WaterTile::WaterTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect, 1, "Water", events, uuid, gameMode, ObstacleType::Water}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	_events->EmitEvent("AnimationCreateWater", _rect);
}

WaterTile::~WaterTile() = default;
