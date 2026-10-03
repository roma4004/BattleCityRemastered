#pragma once

#include "components/EventSystem.h"
#include "components/StatisticsData.h"
#include "components/events/StatisticsEvents.h"
#include <memory>
#include <vector>

enum class Author : char8_t;
struct GameResetEvent;
struct TankDiedEvent;
struct WorldSnapshotRequestedEvent;
struct WorldSnapshotReceivedEvent;
class EventSystem;

class GameStatistics final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	StatisticsData _data{};

	void Subscribe();
	void OnGameReset(const GameResetEvent&);
	void Reset();
	void Credit(Author author, unsigned short EnemyTeamStatistics::*team, unsigned short SeatStatistics::*seat);
	void CreditSeat(Author author, unsigned short SeatStatistics::*seat);

	void OnBulletHit(const StatisticsBulletHitEvent& event);
	void OnTankHit(const StatisticsTankHitEvent& event);
	void OnTankDied(const TankDiedEvent& event);
	void OnBrickWallDied(const BrickWallDiedEvent& event);
	void OnSteelWallDied(const SteelWallDiedEvent& event);
	void OnBonusPickup(const StatisticsBonusPickupEvent& event);
	void OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event);
	void OnBonusExpired(const StatisticsBonusExpiredEvent&);

	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;
	void OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event);

public:
	explicit GameStatistics(const std::shared_ptr<EventSystem>& events);

	[[nodiscard]] const StatisticsData& GetData() const noexcept { return _data; }
};
