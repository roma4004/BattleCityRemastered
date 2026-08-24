#include "entities/obstacles/WaterTile.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "enums/ObstacleType.h"

WaterTile::WaterTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect, 1, "Water", events, uuid, gameMode, ObstacleType::Water, kCollision}
{
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});
}

WaterTile::~WaterTile() = default;

void WaterTile::EmitDeathStatistics(const std::string&, const std::string&) {}
