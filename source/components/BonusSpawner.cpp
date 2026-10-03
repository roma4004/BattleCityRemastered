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
#include "components/WorldSnapshot.h"
#include "entities/bonuses/Bonus.h"
#include "enums/BonusType.h"
#include "utils/RandUtils.h"
#include "utils/UuidUtils.h"
#include "utils/WorldQuery.h"
#include <algorithm>
#include <chrono>
#include <iterator>

class BaseObj;
class EventSystem;

using namespace std::chrono_literals;

namespace
{
//NOTE: one in this many spawns comes out super
constexpr int kSuperBonusOdds{5};
}//namespace

BonusSpawner::BonusSpawner(const std::shared_ptr<EventSystem>& events,
						   const std::vector<std::shared_ptr<BaseObj>>& allObjects, const GameConfig& gameConfig)
	: _events{events}
	, _allObjects{allObjects}
	, _distSpawnType{kFirstSpawnableBonusId, kLastSpawnableBonusId}
	, _distSuperRoll{1, kSuperBonusOdds}
	, _gameConfig{gameConfig}
	, _spawnTimer{60s}
{
	ResetSpawnRanges();

	Subscribe();
}

void BonusSpawner::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BonusSpawner::Reset));
	_subs.push_back(_events->AddListener(this, &BonusSpawner::OnWorldGeometryChanged));
	_subs.push_back(_events->AddListener(this, &BonusSpawner::OnSpawnMapBonus));

	//NOTE: the burst is only a picture on the client - what settles is the host's call, so the bonus
	//waits for BonusSpawnComplete instead of its own clock, and one picked up mid-burst never arrives
	if (_gameConfig.IsAuthority())
	{
		_subs.push_back(_events->AddListener(this, &BonusSpawner::OnSpawnAnimationFinished));
		_subs.push_back(_events->AddListener(this, &BonusSpawner::Update));
	}
	else
	{
		_subs.push_back(_events->AddListener(this, &BonusSpawner::OnBonusSpawned));
		_subs.push_back(_events->AddListener(this, &BonusSpawner::OnBonusSpawnCompleted));
		_subs.push_back(_events->AddListener(this, &BonusSpawner::OnWorldSnapshotReceived));
	}

	if (_gameConfig.IsHost())
	{
		_subs.push_back(_events->AddListener(this, &BonusSpawner::OnWorldSnapshotRequested));
	}
}

void BonusSpawner::OnSpawnAnimationFinished(const SpawnAnimationFinishedEvent& event)
{
	if (MaterializePending(event.uuid) && _gameConfig.IsHost())
	{
		_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = event.uuid});
	}
}

void BonusSpawner::OnBonusSpawnCompleted(const BonusSpawnCompletedEvent& event) { MaterializePending(event.uuid); }

bool BonusSpawner::MaterializePending(const Uuid uuid)
{
	const auto it{std::ranges::find(_pendingSpawns, uuid, &PendingSpawn::uuid)};
	if (it == _pendingSpawns.end())
	{
		return false;
	}

	Materialize(*it);
	_pendingSpawns.erase(it);

	return true;
}

void BonusSpawner::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	std::ranges::transform(_pendingSpawns, std::back_inserter(event.snapshot.bonusSpawns),
						   [](const PendingSpawn& pending)
						   {
							   return BonusSpawnedEvent{.pos = FPoint{.x = pending.rect.x, .y = pending.rect.y},
														.type = pending.type,
														.uuid = pending.uuid,
														.isSuper = pending.isSuper};
						   });
}

//NOTE: a settled bonus lands at once, a pending one bursts and waits for the host like any other
void BonusSpawner::OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event)
{
	const auto size{static_cast<double>(_gameConfig.bonusSize)};
	std::ranges::for_each(event.snapshot.bonuses, [this, size](const BonusSpawnedEvent& bonus)
	{
		Materialize(PendingSpawn{.rect = ObjRectangle{.x = bonus.pos.x, .y = bonus.pos.y, .w = size, .h = size},
								 .type = bonus.type,
								 .uuid = bonus.uuid,
								 .isSuper = bonus.isSuper});
	});
	std::ranges::for_each(event.snapshot.bonusSpawns, [this](const BonusSpawnedEvent& spawn)
	{
		OnBonusSpawned(spawn);
	});
}

void BonusSpawner::OnWorldGeometryChanged(const WorldGeometryChangedEvent&) { ResetSpawnRanges(); }

void BonusSpawner::ResetSpawnRanges()
{
	const UPoint& battlefieldSize{_gameConfig.battlefieldSize};

	_distSpawnPosX = std::uniform_int_distribution<>{0, static_cast<int>(battlefieldSize.x) - _gameConfig.bonusSize};
	_distSpawnPosY = std::uniform_int_distribution<>{0, static_cast<int>(battlefieldSize.y) - _gameConfig.bonusSize};
}

void BonusSpawner::OnBonusSpawned(const BonusSpawnedEvent& event)
{
	const auto size{static_cast<double>(_gameConfig.bonusSize)};
	const ObjRectangle rect{.x = event.pos.x, .y = event.pos.y, .w = size, .h = size};
	SpawnBonus(rect, event.type, event.uuid, event.isSuper);
}

void BonusSpawner::Update(const TickUpdateEvent&)
{
	if (_spawnTimer.IsCooldownFinish())
	{
		const auto size{static_cast<double>(_gameConfig.bonusSize)};
		const auto x{static_cast<double>(RandUtils::GetRandNumber(_distSpawnPosX))};
		const auto y{static_cast<double>(RandUtils::GetRandNumber(_distSpawnPosY))};
		const ObjRectangle rect{.x = x, .y = y, .w = size, .h = size};
		//NOTE: stricter than a tank spawn on purpose - a bonus dropped into a bush is one nobody can see
		if (WorldQuery::IsSpotFree(_allObjects, rect))
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

	if (_gameConfig.IsHost())
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

	_events->EmitEvent(AnimationCreateBonusSpawnEvent{.rect = rect, .uuid = uuid, .isEndless = _gameConfig.IsClient()});
}

void BonusSpawner::SpawnPermanentBonus(const ObjRectangle rect, const BonusType type)
{
	const Uuid uuid{AnnounceSpawn(rect, type, Uuid{}, false)};

	_pendingSpawns.emplace_back(PendingSpawn{.rect = rect, .type = type, .uuid = uuid, .isPermanent = true});

	_events->EmitEvent(AnimationCreateBonusSpawnEvent{.rect = rect, .uuid = uuid, .isEndless = _gameConfig.IsClient()});
}

void BonusSpawner::OnSpawnMapBonus(const SpawnMapBonusEvent& event)
{
	SpawnPermanentBonus(event.rect, event.type);
}

void BonusSpawner::Materialize(const PendingSpawn& pending) const
{
	auto bonus{std::make_shared<Bonus>(pending.rect, _events, pending.uuid, _gameConfig, pending.type,
									   pending.isSuper)};
	_events->EmitEvent(BonusCreatedEvent{.bonus = bonus, .isPermanent = pending.isPermanent});
	_events->EmitEvent(AddToSpawnQueueEvent{.obj = std::move(bonus)});
}

//NOTE: no base to wall in a free-for-all - rolled again
BonusSpawner::RolledBonus BonusSpawner::RollBonus() const
{
	BonusType type{};
	do
	{
		type = static_cast<BonusType>(RandUtils::GetRandNumber(_distSpawnType));
	} while (type == BonusType::Shovel && _gameConfig.IsFreeForAll());

	return RolledBonus{.type = type, .isSuper = RandUtils::GetRandNumber(_distSuperRoll) == 1};
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
