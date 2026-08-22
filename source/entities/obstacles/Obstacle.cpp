#include "entities/obstacles/Obstacle.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/ReplicationEvents.h"
#include "entities/BaseObjProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"

Obstacle::Obstacle(const ObjRectangle rect, const int health, std::string name,
				   const std::shared_ptr<EventSystem>& events, const Uuid uuid, const GameMode gameMode,
				   const ObstacleType obstacleType, const CollisionTags collision)
	: BaseObj{BaseObjProperty{.rect = rect,
							  .health = health,
							  .uuid = uuid,
							  .name = std::move(name),
							  .fraction = "Neutral"},
			  collision}
	, _events(events)
	, _gameMode{gameMode}
	, _obstacleType(obstacleType)
{
	Obstacle::Subscribe();

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(ObstacleSpawnedEvent{.pos = GetPos(), .type = _obstacleType, .uuid = uuid});
	}
}

Obstacle::~Obstacle() = default;

void Obstacle::Subscribe()
{
	if (IsClient(_gameMode))
	{
		Obstacle::SubscribeAsClient();
	}
}

void Obstacle::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Obstacle::OnHealthChanged));
}

void Obstacle::OnHealthChanged(const HealthChangedEvent& event) { SetHealth(event.health); }

void Obstacle::Draw() const
{
	_events->EmitEvent(DrawObjEvent{.rect = _rect, .dir = Direction::UP, .name = _name});
}

void Obstacle::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		EmitDeathStatistics(author, fraction);
	}
}

void Obstacle::TakeDamage(const unsigned int damage, const std::string& author, const std::string& fraction)
{
	BaseObj::TakeDamage(damage, author, fraction);

	SendDamageStatistics(author, fraction);

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(HealthChangedEvent{.who = _name, .health = GetHealth(), .uuid = _uuid});
	}
}
