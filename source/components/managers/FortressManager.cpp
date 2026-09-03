#include "components/managers/FortressManager.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "entities/BaseObj.h"
#include "entities/obstacles/FortressWalls.h"
#include "enums/DespawnReason.h"
#include "enums/ObstacleType.h"
#include "utils/ColliderUtils.h"
#include "utils/WorldQuery.h"
#include "enums/Faction.h"
#include <algorithm>

namespace
{
[[nodiscard]] bool IsSameSpot(const ObjRectangle& lhs, const ObjRectangle& rhs)
{
	return ColliderUtils::AreEqualAbsolute(lhs.x, rhs.x) && ColliderUtils::AreEqualAbsolute(lhs.y, rhs.y);
}
}// namespace

FortressManager::FortressManager(const std::shared_ptr<EventSystem>& events,
								 const std::vector<std::shared_ptr<BaseObj>>& allObjects)
	: _allObjects{allObjects}
	, _events{events}
{
	Subscribe();
}

FortressManager::~FortressManager() = default;

void FortressManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &FortressManager::OnSpotRegistered));
	_subs.push_back(_events->AddListener(this, &FortressManager::OnBonusShovel));
	_subs.push_back(_events->AddListener(this, &FortressManager::OnGameReset));
}

void FortressManager::OnGameReset(const GameResetEvent&) { _spots.clear(); }

void FortressManager::OnSpotRegistered(const FortressSpotRegisteredEvent& event)
{
	const auto it = std::ranges::find_if(_spots, [&event](const Spot& spot)
	{
		return IsSameSpot(spot.rect, event.rect);
	});

	if (it != _spots.end())
	{
		it->wall = event.wall;

		return;
	}

	_spots.push_back(Spot{.rect = event.rect, .wall = event.wall});
}

void FortressManager::ClearSpot(const Spot& spot) const
{
	const std::shared_ptr<BaseObj> wall = spot.wall.lock();
	if (wall == nullptr || !wall->GetIsAlive())
	{
		return;
	}

	//NOTE: bypasses TakeDamage to skip statistics record.
	wall->SetIsAlive(false);
	_events->EmitEvent(DespawnedEvent{.uuid = wall->GetUuid(), .reason = DespawnReason::Destroyed});
}

void FortressManager::Rebuild(const Spot& spot, const ObstacleType material) const
{
	_events->EmitEvent(SpawnFortressWallEvent{.rect = spot.rect, .material = material});
}

void FortressManager::OnBonusShovel(const BonusShovelStatusChangeEvent& event)
{
	for (const Spot& spot: _spots)
	{
		if (event.faction == Faction::EnemyTeam)
		{
			ClearSpot(spot);

			continue;
		}

		if (event.isActive)
		{
			//NOTE: per spot - a tank standing in one gap must not stop the rest from being rebuilt
			if (WorldQuery::IsSpotFreeOfPawns(_allObjects, spot.rect))
			{
				ClearSpot(spot);
				Rebuild(spot, ObstacleType::Steel);
			}

			continue;
		}

		//NOTE: only what the shovel put there comes back - a spot shot out in battle stays empty
		if (dynamic_cast<FortressSteelWall*>(spot.wall.lock().get()) != nullptr)
		{
			ClearSpot(spot);
			Rebuild(spot, ObstacleType::Brick);
		}
	}
}
