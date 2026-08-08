#include "entities/obstacles/SteelWall.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/StatisticsEvents.h"
#include "enums/ObstacleType.h"

SteelWall::SteelWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect, 1, "SteelWall", events, uuid, gameMode, ObstacleType::Steel}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

SteelWall::~SteelWall()
{
	Unsubscribe();
}

void SteelWall::Subscribe()
{
	_events->AddListener(_nameWithUuid, [this](const DrawEvent&) { this->Draw(); });
}

void SteelWall::Unsubscribe() const { _events->RemoveAllListeners(_nameWithUuid); }

void SteelWall::EmitDeathStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(SteelWallDiedEvent{.author = author, .fraction = fraction});
}
