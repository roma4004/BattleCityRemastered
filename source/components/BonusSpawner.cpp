#include "components/BonusSpawner.h"
#include "geometry/Point.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/ReplicationEvents.h"
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
	, _distSpawnType{kFirstSpawnableBonusId, kLastSpawnableBonusId}
	, _distSuperRoll{1, kSuperBonusOdds}
	, _gameConfig{gameConfig}
	, _spawnTimer{std::chrono::seconds{60}}
	, _gameMode{gameConfig.gameMode}
{
	ResetSpawnRanges();

	Subscribe();
}

void BonusSpawner::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BonusSpawner::Reset));
	_subs.push_back(_events->AddListener(this, &BonusSpawner::OnWorldGeometryChanged));

	//NOTE: the burst is only a picture on the client - what settles is the host's call, so the bonus
	//waits for BonusSpawnComplete instead of its own clock, and one picked up mid-burst never arrives
	if (IsAuthority(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &BonusSpawner::OnSpawnAnimationFinished));
		_subs.push_back(_events->AddListener(this, &BonusSpawner::Update));
	}
	else
	{
		_subs.push_back(_events->AddListener(this, &BonusSpawner::OnBonusSpawned));
		_subs.push_back(_events->AddListener(this, &BonusSpawner::OnBonusSpawnCompleted));
	}
}

void BonusSpawner::OnSpawnAnimationFinished(const SpawnAnimationFinishedEvent& event)
{
	//NOTE: the signal goes out only for a bonus that really settled - one already picked up gets none
	if (MaterializePending(event.uuid) && IsHost(_gameMode))
	{
		_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = event.uuid});
	}
}

void BonusSpawner::OnBonusSpawnCompleted(const BonusSpawnCompletedEvent& event) { MaterializePending(event.uuid); }

bool BonusSpawner::MaterializePending(const Uuid uuid)
{
	const auto it = std::ranges::find(_pendingSpawns, uuid, &PendingSpawn::uuid);
	if (it == _pendingSpawns.end())
	{
		return false;
	}

	Materialize(*it);
	_pendingSpawns.erase(it);

	return true;
}

void BonusSpawner::OnWorldGeometryChanged(const WorldGeometryChangedEvent&) { ResetSpawnRanges(); }

void BonusSpawner::ResetSpawnRanges()
{
	const UPoint& battlefieldSize = _gameConfig.battlefieldSize;

	_distSpawnPosX = std::uniform_int_distribution<>{0, static_cast<int>(battlefieldSize.x) - _gameConfig.bonusSize};
	_distSpawnPosY = std::uniform_int_distribution<>{0, static_cast<int>(battlefieldSize.y) - _gameConfig.bonusSize};
}

void BonusSpawner::OnBonusSpawned(const BonusSpawnedEvent& event)
{
	const auto size = static_cast<double>(_gameConfig.bonusSize);
	const ObjRectangle rect{.x = event.pos.x, .y = event.pos.y, .w = size, .h = size};
	SpawnBonus(rect, event.type, event.uuid, event.isSuper);
}

void BonusSpawner::Update(const TickUpdateEvent&)
{
	if (_spawnTimer.IsCooldownFinish())
	{
		const auto size = static_cast<double>(_gameConfig.bonusSize);
		const auto x = static_cast<double>(RandUtils::GetRandNumber(_distSpawnPosX));
		const auto y = static_cast<double>(RandUtils::GetRandNumber(_distSpawnPosY));
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

	_pendingSpawns.emplace_back(PendingSpawn{.rect = rect, .type = type, .uuid = uuid, .isSuper = isSuper});

	_events->EmitEvent(AnimationCreateBonusSpawnEvent{.rect = rect, .uuid = uuid});
}

void BonusSpawner::Materialize(const PendingSpawn& pending) const
{
	auto bonus = std::make_shared<Bonus>(pending.rect, _events, pending.uuid, _gameMode, pending.type,
										 pending.isSuper);
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
