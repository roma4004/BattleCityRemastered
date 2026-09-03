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
#include "enums/TextureType.h"

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
{}

Obstacle::~Obstacle() = default;

void Obstacle::Activate()
{
	Subscribe();

	if (IsClient(_gameMode))
	{
		SubscribeAsClient();
	}
}

void Obstacle::Deactivate() { _subs.clear(); }

void Obstacle::Subscribe() {}

void Obstacle::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Obstacle::OnHealthChanged));
	//NOTE: how a wall the shovel swept away reaches the client - it never took damage, so no health
	//change is coming for it
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Obstacle::OnDespawned));
}

void Obstacle::OnHealthChanged(const HealthChangedEvent& event) { SetHealth(event.health); }

void Obstacle::OnDespawned(const DespawnedEvent&) { SetIsAlive(false); }

namespace
{
//NOTE: Water animates instead of drawing a still, and Fortress never reaches an instance - the ring
//around the eagle is built out of brick and steel walls
[[nodiscard]] constexpr TextureType TextureOf(const ObstacleType type)
{
	switch (type)
	{
		case ObstacleType::Brick:
			return TextureType::BrickWall;
		case ObstacleType::Steel:
			return TextureType::SteelWall;
		case ObstacleType::Eagle:
			return TextureType::Eagle;
		case ObstacleType::Bush:
			return TextureType::Bush;
		case ObstacleType::Ice:
			return TextureType::Ice;
		case ObstacleType::None:
		case ObstacleType::Fortress:
		case ObstacleType::Water:
		case ObstacleType::lastId:
			break;
	}

	return TextureType::None;
}
}//namespace

void Obstacle::Draw() const
{
	_events->EmitEvent(DrawObjEvent{.rect = _rect, .dir = Direction::UP, .texture = TextureOf(_obstacleType)});
}

void Obstacle::TakeDamage(const unsigned int damage, const Author author)
{
	if (!GetIsAlive())
	{
		return;
	}

	BaseObj::TakeDamage(damage, author);

	if (!IsHost(_gameMode))
	{
		return;
	}

	_events->EmitEvent(HealthChangedEvent{.health = GetHealth(), .uuid = _uuid});

	if (!GetIsAlive())
	{
		_events->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::Destroyed});
	}
}
