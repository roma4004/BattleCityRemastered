#include "entities/obstacles/EagleTile.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/ObstacleType.h"

EagleTile::EagleTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect, 1, "Eagle", events, uuid, gameMode, ObstacleType::Eagle}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

EagleTile::~EagleTile()
{
	_events->EmitEvent(PlayersBaseFinishedEvent{});
}

void EagleTile::Subscribe()
{
	_subs.push_back(_events->AddListener(_nameWithUuid, [this](const DrawEvent&) { this->Draw(); }));
}

void EagleTile::EmitDeathStatistics(const std::string&, const std::string&) {}
