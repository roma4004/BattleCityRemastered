#include "entities/obstacles/Obstacle.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "entities/BaseObjProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"
#include "enums/Faction.h"

Obstacle::Obstacle(const ObjRectangle rect, const int health, std::string name,
				   const std::shared_ptr<EventSystem>& events, const Uuid uuid, const GameMode gameMode,
				   const ObstacleType obstacleType, const CollisionTags collision)
	: BaseObj{BaseObjProperty{.rect = rect,
							  .health = health,
							  .uuid = uuid,
							  .name = std::move(name),
							  .faction = Faction::Neutral},
			  collision}
	, _events(events)
	, _gameMode{gameMode}
	, _obstacleType(obstacleType)
{
	//NOTE: safe in a constructor only because nothing here is virtual - a leaf adds its own
	//subscriptions from its own constructor, once it is complete
	if (IsClient(_gameMode))
	{
		SubscribeAsClient();
	}
}

Obstacle::~Obstacle() = default;

void Obstacle::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Obstacle::OnHealthChanged));
	//NOTE: how a wall the shovel swept away reaches the client - it never took damage, so no health
	//change is coming for it
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Obstacle::OnDespawned));
}

void Obstacle::OnHealthChanged(const HealthChangedEvent& event) { SetHealth(event.health); }

void Obstacle::OnDespawned(const DespawnedEvent&) { SetIsAlive(false); }

void Obstacle::Draw() const
{
	_events->EmitEvent(DrawObjEvent{.rect = _rect, .dir = Direction::UP, .name = _name});
}

void Obstacle::TakeDamage(const unsigned int damage, const std::string& author, Faction faction)
{
	if (!GetIsAlive())
	{
		return;
	}

	BaseObj::TakeDamage(damage, author, faction);

	if (!IsHost(_gameMode))
	{
		return;
	}

	_events->EmitEvent(HealthChangedEvent{.who = _name, .health = GetHealth(), .uuid = _uuid});

	if (!GetIsAlive())
	{
		_events->EmitEvent(DespawnedEvent{.who = _name, .uuid = _uuid, .reason = DespawnReason::Destroyed});
	}
}
