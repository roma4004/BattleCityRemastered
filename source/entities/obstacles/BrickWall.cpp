#include "entities/obstacles/BrickWall.h"
#include "components/EventSystem.h"
#include "components/events/StatisticsEvents.h"
#include "enums/ObstacleType.h"
#include "application/GameConfig.h"

BrickWall::BrickWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
					 const GameConfig& gameConfig)
	: Obstacle{rect, kWallHealth, events, uuid, gameConfig, ObstacleType::Brick, kCollision, kLayer}
{}

void BrickWall::EmitDeathStatistics(const Author author)
{
	_events->EmitEvent(BrickWallDiedEvent{.author = author});
}
