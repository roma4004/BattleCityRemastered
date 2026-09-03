#include "entities/obstacles/SteelWall.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/StatisticsEvents.h"
#include "enums/ObstacleType.h"

SteelWall::SteelWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect, 1, events, uuid, gameMode, ObstacleType::Steel, kCollision}
{}

void SteelWall::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &SteelWall::OnDraw));
}

void SteelWall::OnDraw(const DrawEvent&) const { Draw(); }

void SteelWall::EmitDeathStatistics(const Author author)
{
	_events->EmitEvent(SteelWallDiedEvent{.author = author});
}
