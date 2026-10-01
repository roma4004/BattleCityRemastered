#include "entities/obstacles/EagleTile.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "enums/ObstacleType.h"
#include "application/GameConfig.h"

EagleTile::EagleTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
					 const GameConfig& gameConfig)
	: Obstacle{rect, kHealth, events, uuid, gameConfig, ObstacleType::Eagle, kCollision}
{}

void EagleTile::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &EagleTile::OnDraw));
	_subs.push_back(_events->AddListener(this, &EagleTile::OnPostDraw));

	if (_gameConfig.IsAuthority())
	{
		_subs.push_back(_events->AddListener(this, &EagleTile::OnBonusShovel));
	}
}

void EagleTile::OnDraw(const DrawEvent&) const { Draw(); }

//NOTE: a whole eagle wears no bar - it shows up with the first hit
void EagleTile::OnPostDraw(const PostDrawEvent&) const
{
	if (GetHealth() < kHealth)
	{
		_events->EmitEvent(RenderHealthBarEvent{.rect = GetRect(), .health = GetHealth()});
	}
}

//NOTE: the players' shovel heals the eagle at once; the enemies' arrives switched off and only sweeps the walls away
void EagleTile::OnBonusShovel(const BonusShovelStatusChangeEvent& event)
{
	if (!event.isActive)
	{
		return;
	}

	SetHealth(kHealth);

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(HealthChangedEvent{.health = GetHealth(), .uuid = _uuid});
	}
}

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
