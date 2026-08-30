#pragma once

#include "components/EventSystem.h"
#include "components/events/StatisticsEvents.h"
#include <memory>
#include <string>
#include <vector>

enum class Faction : char8_t;

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

	void OnBulletHit(const StatisticsBulletHitEvent& event);
	void OnTankHit(const StatisticsTankHitEvent& event);
	void OnTankDied(const TankDiedEvent& event);
	void OnBrickWallDied(const BrickWallDiedEvent& event);
	void OnSteelWallDied(const SteelWallDiedEvent& event);
	void OnBonusPickup(const StatisticsBonusPickupEvent& event);
	void OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event);
	void OnBonusExpired(const StatisticsBonusExpiredEvent&);

	void OnEnemyHit(const std::string& author, Faction faction);
	void OnPlayerOneHit(const std::string& author, Faction faction);
	void OnPlayerTwoHit(const std::string& author, Faction faction);
	void OnEnemyDied(const std::string& author, Faction faction);
	void OnPlayerOneDied(const std::string& author, Faction faction);
	void OnPlayerTwoDied(const std::string& author, Faction faction);

public:
	explicit GameStatistics(const std::shared_ptr<EventSystem>& events);

	~GameStatistics() = default;

	void Reset();

	[[nodiscard]] unsigned short GetBulletHitByEnemy() const { return _data.bulletHitByEnemy; }
	[[nodiscard]] unsigned short GetBulletHitByPlayerOne() const { return _data.bulletHitByPlayerOne; }
	[[nodiscard]] unsigned short GetBulletHitByPlayerTwo() const { return _data.bulletHitByPlayerTwo; }

	[[nodiscard]] unsigned short GetEnemyHitByFriendlyFire() const { return _data.enemyHitByFriendlyFire; }
	[[nodiscard]] unsigned short GetEnemyHitByPlayerOne() const { return _data.enemyHitByPlayerOne; }
	[[nodiscard]] unsigned short GetEnemyHitByPlayerTwo() const { return _data.enemyHitByPlayerTwo; }

	[[nodiscard]] unsigned short GetPlayerOneHitFriendlyFire() const { return _data.playerOneHitFriendlyFire; }
	[[nodiscard]] unsigned short GetPlayerOneHitByEnemyTeam() const { return _data.playerOneHitByEnemyTeam; }

	[[nodiscard]] unsigned short GetPlayerTwoHitFriendlyFire() const { return _data.playerTwoHitFriendlyFire; }
	[[nodiscard]] unsigned short GetPlayerTwoHitByEnemyTeam() const { return _data.playerTwoHitByEnemyTeam; }

	[[nodiscard]] unsigned short GetEnemyDiedByFriendlyFire() const { return _data.enemyDiedByFriendlyFire; }
	[[nodiscard]] unsigned short GetEnemyDiedByPlayerOne() const { return _data.enemyDiedByPlayerOne; }
	[[nodiscard]] unsigned short GetEnemyDiedByPlayerTwo() const { return _data.enemyDiedByPlayerTwo; }

	[[nodiscard]] unsigned short GetPlayerOneDiedByFriendlyFire() const { return _data.playerOneDiedByFriendlyFire; }
	[[nodiscard]] unsigned short GetPlayerTwoDiedByFriendlyFire() const { return _data.playerTwoDiedByFriendlyFire; }
	[[nodiscard]] unsigned short GetPlayerDiedByEnemyTeam() const { return _data.playerDiedByEnemyTeam; }

	[[nodiscard]] unsigned short GetBrickWallDiedByEnemyTeam() const { return _data.brickWallDiedByEnemyTeam; }
	[[nodiscard]] unsigned short GetBrickWallDiedByPlayerOne() const { return _data.brickWallDiedByPlayerOne; }
	[[nodiscard]] unsigned short GetBrickWallDiedByPlayerTwo() const { return _data.brickWallDiedByPlayerTwo; }

	[[nodiscard]] unsigned short GetSteelWallDiedByEnemyTeam() const { return _data.steelWallDiedByEnemyTeam; }
	[[nodiscard]] unsigned short GetSteelWallDiedByPlayerOne() const { return _data.steelWallDiedByPlayerOne; }
	[[nodiscard]] unsigned short GetSteelWallDiedByPlayerTwo() const { return _data.steelWallDiedByPlayerTwo; }

	[[nodiscard]] unsigned short GetBonusPickupByEnemyTeam() const { return _data.bonusPickupByEnemyTeam; }
	[[nodiscard]] unsigned short GetBonusPickupByPlayerOne() const { return _data.bonusPickupByPlayerOne; }
	[[nodiscard]] unsigned short GetBonusPickupByPlayerTwo() const { return _data.bonusPickupByPlayerTwo; }

	[[nodiscard]] unsigned short GetBonusDestroyedByEnemyTeam() const { return _data.bonusDestroyedByEnemyTeam; }
	[[nodiscard]] unsigned short GetBonusDestroyedByPlayerOne() const { return _data.bonusDestroyedByPlayerOne; }
	[[nodiscard]] unsigned short GetBonusDestroyedByPlayerTwo() const { return _data.bonusDestroyedByPlayerTwo; }

	[[nodiscard]] unsigned short GetBonusExpired() const { return _data.bonusExpired; }
};
