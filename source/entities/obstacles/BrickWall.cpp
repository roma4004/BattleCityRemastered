#include "entities/obstacles/BrickWall.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/StatisticsEvents.h"
#include "enums/ObstacleType.h"
#include "application/GameConfig.h"

BrickWall::BrickWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
					 const GameConfig& gameConfig)
	: Obstacle{rect, 1, events, uuid, gameConfig, ObstacleType::Brick, kCollision}
{}

void BrickWall::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BrickWall::OnDraw));
}

void BrickWall::OnDraw(const DrawEvent&) const { Draw(); }

void BrickWall::EmitDeathStatistics(const Author author)
{
	_events->EmitEvent(BrickWallDiedEvent{.author = author});
}
