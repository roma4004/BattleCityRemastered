#include "components/BulletPool.h"
#include "utils/Log.h"
#include "utils/UuidUtils.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/PawnProperty.h"

BulletPool::BulletPool(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
					   const GameConfig& gameConfig)
	: _events{events}
	, _allObjects{allObjects}
	, _gameConfig{gameConfig}
{
	// Pre-generate 20 default bullets
	for (size_t i = 0u; i < 20u; ++i)
	{
		_bullets.push(CreateNewBullet());
	}

	Subscribe();
}

void BulletPool::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BulletPool::OnGameReset));
}

void BulletPool::OnGameReset(const GameResetEvent&) { Clear(); }

std::shared_ptr<Bullet> BulletPool::CreateNewBullet()
{
	PawnProperty pawnProperty{.baseObjProperty = {},
							  .allObjects = _allObjects,
							  .events = _events,
							  .gameMode = _gameConfig.gameMode};

	return {new Bullet{std::move(pawnProperty), _gameConfig}, [this](Bullet* b) { ReturnBullet(b); }};
}

std::shared_ptr<BaseObj> BulletPool::SpawnBullet()
{
	std::scoped_lock lock(_bulletsMutex);

	std::shared_ptr<BaseObj> bullet;
	if (_bullets.empty())
	{
		bullet = CreateNewBullet();
	}
	else
	{
		bullet = _bullets.front();
		_bullets.pop();
	}

	return bullet;
}

void BulletPool::ReturnBullet(BaseObj* bullet)
{
	if (_isClearing)
	{
		delete bullet;

		return;
	}

	std::scoped_lock lock(_bulletsMutex);
	if (auto* bulletCast = dynamic_cast<Bullet*>(bullet); bulletCast != nullptr)
	{
		Log::Detail("bullet returned to a pool of " + std::to_string(_bullets.size()) + ", author "
					+ bulletCast->GetAuthor() + " uuid " + UuidUtils::GetStringUuid(bulletCast->GetUuid()));

		bulletCast->Disable();
		_bullets.emplace(std::shared_ptr<BaseObj>(bullet, [this](BaseObj* b)
		{
			ReturnBullet(b);
		}));

		_events->EmitEvent(DespawnedEvent{
				.who = bulletCast->GetName(), .uuid = bulletCast->GetUuid(), .reason = DespawnReason::Destroyed});
	}
}

void BulletPool::Clear()
{
	std::scoped_lock lock(_bulletsMutex);
	_isClearing = true;

	Log::Detail("bullet pool cleared, held " + std::to_string(_bullets.size()));

	while (!_bullets.empty())
	{
		_bullets.pop();
	}

	_isClearing = false;
}
