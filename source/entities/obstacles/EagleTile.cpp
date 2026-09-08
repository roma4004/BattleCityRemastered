#include "entities/obstacles/EagleTile.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/ObstacleType.h"
#include "application/GameConfig.h"

EagleTile::EagleTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
					 const GameConfig& gameConfig)
	: Obstacle{rect, 1, events, uuid, gameConfig, ObstacleType::Eagle, kCollision}
{}

void EagleTile::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &EagleTile::OnDraw));
}

void EagleTile::OnDraw(const DrawEvent&) const { Draw(); }

//NOTE: the base falls where it is destroyed - a field wiped on reset must not read as a defeat
void EagleTile::EmitDeathStatistics(Author)
{
	_events->EmitEvent(PlayersBaseFinishedEvent{});
}

//NOTE: the client is told, it does not work it out from health
void EagleTile::OnDespawned(const DespawnedEvent& event)
{
	Obstacle::OnDespawned(event);

	_events->EmitEvent(PlayersBaseFinishedEvent{});
}
