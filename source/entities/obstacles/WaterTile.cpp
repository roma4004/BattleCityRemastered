#include "entities/obstacles/WaterTile.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "enums/ObstacleType.h"
#include "application/GameConfig.h"

WaterTile::WaterTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
					 const GameConfig& gameConfig)
	: Obstacle{rect, 1, events, uuid, gameConfig, ObstacleType::Water, kCollision}
{
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});
}

void WaterTile::EmitDeathStatistics(Author) {}
