#include "entities/obstacles/EagleTile.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/ObstacleType.h"

EagleTile::EagleTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect, 1, "Eagle", events, uuid, gameMode, ObstacleType::Eagle, s_collision}
{
	Subscribe();
}

EagleTile::~EagleTile()
{
	_events->EmitEvent(PlayersBaseFinishedEvent{});
}

void EagleTile::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &EagleTile::OnDraw));
}

void EagleTile::OnDraw(const DrawEvent&) const { Draw(); }

void EagleTile::EmitDeathStatistics(const std::string&, const std::string&) {}
