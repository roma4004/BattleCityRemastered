#include "components/BonusSpawner.h"
#include "geometry/Point.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/TimingEvents.h"
#include "components/events/SpawnEvents.h"
#include "entities/bonuses/Bonus.h"
#include "enums/BonusType.h"
#include "enums/GameMode.h"
#include "utils/ColliderUtils.h"
#include "utils/RandUtils.h"
#include "utils/UuidUtils.h"
#include <algorithm>

class BaseObj;
class EventSystem;

namespace
{
//NOTE: one in this many spawns comes out super
constexpr int kSuperBonusOdds{5};
}//namespace

BonusSpawner::BonusSpawner(const std::shared_ptr<EventSystem>& events,
						   std::vector<std::shared_ptr<BaseObj>>* allObjects, const GameConfig& gameConfig)
	: _events{events}
	, _allObjects{allObjects}
	, _distSpawnPosY{0, static_cast<int>(gameConfig.windowSize.y) - gameConfig.bonusSize}
	, _distSpawnPosX{0, static_cast<int>(gameConfig.windowSize.x - gameConfig.sideBarWidth) - gameConfig.bonusSize}
	, _distSpawnType{kFirstSpawnableBonusId, kLastSpawnableBonusId}
	, _distSuperRoll{1, kSuperBonusOdds}
	, _gameConfig{gameConfig}
	, _spawnTimer{std::chrono::seconds{60}}
	, _gameMode{gameConfig.gameMode}
{
	Subscribe();
}

void BonusSpawner::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BonusSpawner::Reset));
	_subs.push_back(_events->AddListener(this, &BonusSpawner::OnWorldGeometryChanged));

	//NOTE: unlike a tank, a bonus runs its burst on both sides - the client is handed the pick up front
	//so it plays the whole animation, not just its tail
	_subs.push_back(_events->AddListener(this, &BonusSpawner::OnSpawnAnimationFinished));

	if (IsAuthority(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &BonusSpawner::Update));
	}
	else
	{
		_subs.push_back(_events->AddListener(this, &BonusSpawner::OnBonusSpawned));
	}
}

void BonusSpawner::OnSpawnAnimationFinished(const SpawnAnimationFinishedEvent& event)
{
	const auto it = std::ranges::find(_pendingSpawns, event.uuid, &PendingSpawn::uuid);
	if (it == _pendingSpawns.end())
	{
		return;
	}

	Materialize(*it);
	_pendingSpawns.erase(it);
}

//NOTE: on the geometry, not on the window - bonusSize and sideBarWidth are only settled once the
//new cell size has been worked out, and this event is emitted after that
void BonusSpawner::OnWorldGeometryChanged(const WorldGeometryChangedEvent&)
{
	const UPoint& windowSize = _gameConfig.windowSize;
	_distSpawnPosY = std::uniform_int_distribution<>{
			0,
			static_cast<int>(windowSize.y) - _gameConfig.bonusSize};
	_distSpawnPosX = std::uniform_int_distribution<>{
			0,
			static_cast<int>(windowSize.x - _gameConfig.sideBarWidth) - _gameConfig.bonusSize};
}

void BonusSpawner::OnBonusSpawned(const BonusSpawnedEvent& event)
{
	const auto size = static_cast<float>(_gameConfig.bonusSize);
	const ObjRectangle rect{.x = event.pos.x, .y = event.pos.y, .w = size, .h = size};
	SpawnBonus(rect, event.type, event.uuid, event.isSuper);
}

void BonusSpawner::Update(const TickUpdateEvent&)
{
	if (_spawnTimer.IsCooldownFinish())
	{
		const auto size = static_cast<float>(_gameConfig.bonusSize);
		const auto x = static_cast<float>(RandUtils::GetRandNumber(_distSpawnPosX));
		const auto y = static_cast<float>(RandUtils::GetRandNumber(_distSpawnPosY));
		const ObjRectangle rect{.x = x, .y = y, .w = size, .h = size};
		const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
		{
			return ColliderUtils::IsCollide(rect, object->GetRect());
		});

		if (isFreeSpawnSpot)
		{
			SpawnRandomBonus(rect);
			_spawnTimer.Reset();
		}
	}
}

Uuid BonusSpawner::AnnounceSpawn(const ObjRectangle rect, const BonusType type, Uuid uuid, const bool isSuper) const
{
	if (uuid == UuidUtils::GetNilUuid())
	{
		uuid = UuidUtils::GetRandomUuid();
	}

	if (IsHost(_gameMode))
	{
		//NOTE: announced when the spot is picked, not when the bonus lands - the client needs the whole
		//animation, not just its tail
		_events->EmitEvent(BonusSpawnedEvent{.pos = FPoint{.x = rect.x, .y = rect.y},
											 .type = type,
											 .uuid = uuid,
											 .isSuper = isSuper});
	}

	return uuid;
}

void BonusSpawner::SpawnBonus(const ObjRectangle rect, const BonusType type, Uuid uuid, const bool isSuper)
{
	uuid = AnnounceSpawn(rect, type, uuid, isSuper);

	//NOTE: queued before the burst starts - the bonus lands when its last frame is done
	_pendingSpawns.emplace_back(PendingSpawn{.rect = rect, .type = type, .uuid = uuid, .isSuper = isSuper});
	_events->EmitEvent(AnimationCreateBonusSpawnEvent{.rect = rect, .uuid = uuid});
}

void BonusSpawner::Materialize(const PendingSpawn& pending) const
{
	const auto& [rect, type, uuid, isSuper] = pending;

	auto bonus = std::make_shared<Bonus>(rect, _events, uuid, _gameMode, type, isSuper);
	_events->EmitEvent(BonusCreatedEvent{.bonus = bonus});
	_events->EmitEvent(AddToSpawnQueueEvent{.obj = std::shared_ptr<BaseObj>{std::move(bonus)}});
}

BonusSpawner::RolledBonus BonusSpawner::RollBonus()
{
	return RolledBonus{.type = static_cast<BonusType>(RandUtils::GetRandNumber(_distSpawnType)),
					   .isSuper = RandUtils::GetRandNumber(_distSuperRoll) == 1};
}

void BonusSpawner::SpawnRandomBonus(const ObjRectangle rect)
{
	const auto [type, isSuper] = RollBonus();
	SpawnBonus(rect, type, {}, isSuper);
}

void BonusSpawner::Reset(const GameResetEvent&)
{
	_spawnTimer.Reset();
	_pendingSpawns.clear();
}
