#pragma once

#include "components/EventSystem.h"
#include "components/events/StatisticsEvents.h"
#include <memory>
#include <vector>

class EventSystem;
struct GameResetEvent;
struct TankDiedEvent;

struct StatisticsData final
{
	unsigned short bulletHitByEnemy{};
	unsigned short bulletHitByPlayerOne{};
	unsigned short bulletHitByPlayerTwo{};

	unsigned short enemyHitByFriendlyFire{};
	unsigned short enemyHitByPlayerOne{};
	unsigned short enemyHitByPlayerTwo{};

	unsigned short playerOneHitFriendlyFire{};
	unsigned short playerOneHitByEnemyTeam{};

	unsigned short playerTwoHitFriendlyFire{};
	unsigned short playerTwoHitByEnemyTeam{};

	unsigned short enemyDiedByFriendlyFire{};
	unsigned short enemyDiedByPlayerOne{};
	unsigned short enemyDiedByPlayerTwo{};

	unsigned short playerOneDiedByFriendlyFire{};
	unsigned short playerTwoDiedByFriendlyFire{};
	unsigned short playerDiedByEnemyTeam{};

	unsigned short brickWallDiedByEnemyTeam{};
	unsigned short brickWallDiedByPlayerOne{};
	unsigned short brickWallDiedByPlayerTwo{};

	unsigned short steelWallDiedByEnemyTeam{};
	unsigned short steelWallDiedByPlayerOne{};
	unsigned short steelWallDiedByPlayerTwo{};

	unsigned short bonusPickupByEnemyTeam{};
	unsigned short bonusPickupByPlayerOne{};
	unsigned short bonusPickupByPlayerTwo{};

	unsigned short bonusDestroyedByEnemyTeam{};
	unsigned short bonusDestroyedByPlayerOne{};
	unsigned short bonusDestroyedByPlayerTwo{};

	unsigned short bonusExpired{};
};

class GameStatistics final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	StatisticsData _data{};

	void Subscribe();
	void OnGameReset(const GameResetEvent&);
	void Reset();

	void OnBulletHit(const StatisticsBulletHitEvent& event);
	void OnTankHit(const StatisticsTankHitEvent& event);
	void OnTankDied(const TankDiedEvent& event);
	void OnBrickWallDied(const BrickWallDiedEvent& event);
	void OnSteelWallDied(const SteelWallDiedEvent& event);
	void OnBonusPickup(const StatisticsBonusPickupEvent& event);
	void OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event);
	void OnBonusExpired(const StatisticsBonusExpiredEvent&);

public:
	explicit GameStatistics(const std::shared_ptr<EventSystem>& events);

	~GameStatistics() = default;

	//NOTE: read-only view of the whole block - thirty one-line getters said nothing the field names
	//do not, and the scoreboard walks these by pointer-to-member
	[[nodiscard]] const StatisticsData& GetData() const { return _data; }
};
