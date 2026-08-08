#include "entities/obstacles/BrickWall.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/StatisticsEvents.h"
#include "enums/ObstacleType.h"

BrickWall::BrickWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect, 1, {"BrickWall"}, events, uuid, gameMode, ObstacleType::Brick}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

BrickWall::~BrickWall()
{
	Unsubscribe();
}

void BrickWall::Subscribe()
{
	_events->AddListener(_nameWithUuid, [this](const DrawEvent&) { this->Draw(); });
}

void BrickWall::Unsubscribe() const
{
	_events->RemoveListener<DrawEvent>(_nameWithUuid);
}

void BrickWall::EmitDeathStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(BrickWallDiedEvent{.author = author, .fraction = fraction});
}
