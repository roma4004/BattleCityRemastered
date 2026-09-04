#include "components/BulletPool.h"
#include "utils/Log.h"
#include "utils/UuidUtils.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"
#include <optional>

BulletPool::BulletPool(const std::shared_ptr<EventSystem>& events,
					   const std::vector<std::shared_ptr<BaseObj>>& allObjects,
					   const GameConfig& gameConfig)
	: _events{events}
	, _allObjects{allObjects}
	, _gameConfig{gameConfig}
{
	// Pre-generate 20 default bullets
	for (size_t i = 0u; i < 20u; ++i)
	{
		_slots.AddFree(CreateNewBullet());
	}

	Subscribe();
}

void BulletPool::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BulletPool::OnGameReset));
	_subs.push_back(_events->AddListener(this, &BulletPool::OnPostTickUpdate));
}

//NOTE: nothing announced for what it takes back - the world is going away with the listeners
void BulletPool::OnGameReset(const GameResetEvent&)
{
	_slots.ReclaimAll();

	Log::Detail("bullet pool shelved for a new match, free " + std::to_string(_slots.FreeCount()));
}

std::shared_ptr<Bullet> BulletPool::CreateNewBullet() const
{
	PawnProperty pawnProperty{.baseObjProperty = {},
							  .allObjects = _allObjects,
							  .events = _events};

	return std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig);
}

std::shared_ptr<Bullet> BulletPool::SpawnBullet(const BulletResetProperty& property, const std::optional<Uuid>& uuid)
{
	std::shared_ptr<Bullet> bullet = _slots.TakeFree();
	if (bullet == nullptr)
	{
		bullet = CreateNewBullet();
	}

	//NOTE: the pool names the shot, not the shooter - a reused slot would otherwise fire under the
	//uuid of the bullet before it. Safe here because a pooled bullet is unsubscribed until Reset
	bullet->SetId(uuid.value_or(UuidUtils::GetRandomUuid()));
	bullet->Reset(property);

	_slots.Track(bullet);

	return bullet;
}

//NOTE: the same frame step that takes a dead object out of _allObjects - a bullet spent this frame
//is back on the free list before anything can shoot again
void BulletPool::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	//NOTE: the pool reclaims itself rather than relying on SpawnManager having swept the bullet
	//earlier in this same PostTickUpdate
	const std::vector<std::shared_ptr<Bullet>> returned = _slots.ReclaimDead();

	for (const std::shared_ptr<Bullet>& bullet: returned)
	{
		Log::Detail("bullet returned to a pool of " + std::to_string(_slots.FreeCount()) + ", author "
					+ std::string{ToString(bullet->GetAuthor())} + " uuid "
					+ UuidUtils::GetStringUuid(bullet->GetUuid()));
	}

	//NOTE: announced only once every slot is back - a listener is free to shoot again
	for (const std::shared_ptr<Bullet>& bullet: returned)
	{
		_events->EmitEvent(DespawnedEvent{.uuid = bullet->GetUuid(), .reason = DespawnReason::Destroyed});
	}
}
