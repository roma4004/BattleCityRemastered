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
#include <algorithm>
#include <iterator>
#include <optional>

BulletPool::BulletPool(const std::shared_ptr<EventSystem>& events, const std::vector<std::shared_ptr<BaseObj>>& allObjects,
					   const GameConfig& gameConfig)
	: _events{events}
	, _allObjects{allObjects}
	, _gameConfig{gameConfig}
{
	// Pre-generate 20 default bullets
	for (size_t i = 0u; i < 20u; ++i)
	{
		_free.push(CreateNewBullet());
	}

	Subscribe();
}

void BulletPool::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BulletPool::OnGameReset));
	_subs.push_back(_events->AddListener(this, &BulletPool::OnPostTickUpdate));
}

void BulletPool::OnGameReset(const GameResetEvent&) { Clear(); }

std::shared_ptr<Bullet> BulletPool::CreateNewBullet() const
{
	PawnProperty pawnProperty{.baseObjProperty = {},
							  .allObjects = _allObjects,
							  .events = _events,
							  .gameMode = _gameConfig.gameMode};

	return std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig);
}

std::shared_ptr<Bullet> BulletPool::SpawnBullet(const BulletResetProperty& property, const std::optional<Uuid>& uuid)
{
	std::scoped_lock lock(_bulletsMutex);

	std::shared_ptr<Bullet> bullet;
	if (_free.empty())
	{
		bullet = CreateNewBullet();
	}
	else
	{
		bullet = _free.front();
		_free.pop();
	}

	//NOTE: the pool names the shot, not the shooter - a reused slot would otherwise fire under the
	//uuid of the bullet before it. Safe here because a pooled bullet is unsubscribed until Reset
	bullet->SetId(uuid.value_or(UuidUtils::GetRandomUuid()));
	bullet->Reset(property);

	_inFlight.push_back(bullet);

	return bullet;
}

//NOTE: the same frame step that takes a dead object out of _allObjects - a bullet spent this frame
//is back on the free list before anything can shoot again
void BulletPool::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	auto isSpent = [](const std::shared_ptr<Bullet>& bullet) { return !bullet->GetIsAlive(); };

	std::vector<std::shared_ptr<Bullet>> returned{};
	{
		std::scoped_lock lock(_bulletsMutex);

		std::ranges::copy_if(_inFlight, std::back_inserter(returned), isSpent);
		std::erase_if(_inFlight, isSpent);

		for (const std::shared_ptr<Bullet>& bullet: returned)
		{
			Log::Detail("bullet returned to a pool of " + std::to_string(_free.size()) + ", author "
						+ std::string{ToString(bullet->GetAuthor())} + " uuid "
						+ UuidUtils::GetStringUuid(bullet->GetUuid()));

			//NOTE: the pool guarantees it itself rather than relying on SpawnManager having swept
			//the bullet earlier in this same PostTickUpdate - a bullet in _free must not listen,
			//or the next SpawnBullet would rename a subscribed one and subscribe it twice
			bullet->Deactivate();
			_free.push(bullet);
		}
	}

	//NOTE: announced with the lock released - a listener is free to shoot back
	for (const std::shared_ptr<Bullet>& bullet: returned)
	{
		_events->EmitEvent(DespawnedEvent{.uuid = bullet->GetUuid(), .reason = DespawnReason::Destroyed});
	}
}

void BulletPool::Clear()
{
	std::scoped_lock lock(_bulletsMutex);

	Log::Detail("bullet pool cleared, held " + std::to_string(_free.size() + _inFlight.size()));

	_free = {};
	_inFlight.clear();
}
