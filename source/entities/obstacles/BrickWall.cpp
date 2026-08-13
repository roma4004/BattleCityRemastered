#include "entities/obstacles/BrickWall.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/StatisticsEvents.h"
#include "enums/ObstacleType.h"

BrickWall::BrickWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect, 1, {"BrickWall"}, events, uuid, gameMode, ObstacleType::Brick, s_collision}
{
	Subscribe();
}

void BrickWall::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BrickWall::OnDraw));
}

void BrickWall::OnDraw(const DrawEvent&) const { Draw(); }

void BrickWall::EmitDeathStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(BrickWallDiedEvent{.author = author, .fraction = fraction});
}
