#pragma once

#include "components/EventSystem.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/StatisticsEvents.h"
#include <memory>
#include <string>
#include <vector>

enum class GameMode : char8_t;
class EventSystem;
struct GameResetEvent;
struct GameModeChangedToEvent;

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
};

class GameStatistics final
{
	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime on every GameModeChangedToEvent, independent of _subs's fixed
	// subscribe-once-at-construction lifetime - clearing one of these vectors auto-unsubscribes
	// just that group.
	std::vector<EventSubscription> _hostSubs{};
	std::vector<EventSubscription> _clientSubs{};
	StatisticsData _data{};
	GameMode _gameMode{};

	void Subscribe();
	void SubscribeHost();
	void SubscribeAsClient();
	void OnGameReset(const GameResetEvent&);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);

	void UnsubscribeAsHost();
	void UnsubscribeAsClient();

	void OnBulletHit(const StatisticsBulletHitEvent& event);
	void OnClientInBulletHit(const ClientInBulletHitEvent& event);
	void OnEnemyHit(const std::string& author, const std::string& fraction);
	void OnClientInEnemyHit(const ClientInEnemyHitEvent& event);
	void OnPlayerOneHit(const std::string& author, const std::string& fraction);
	void OnClientInPlayerOneHit(const ClientInPlayerOneHitEvent& event);
	void OnPlayerTwoHit(const std::string& author, const std::string& fraction);
	void OnClientInPlayerTwoHit(const ClientInPlayerTwoHitEvent& event);
	void OnTankHit(const StatisticsTankHitEvent& event);
	void OnEnemyDied(const std::string& author, const std::string& fraction);
	void OnClientInEnemyDied(const ClientInEnemyDiedEvent& event);
	void OnPlayerOneDied(const std::string& author, const std::string& fraction);
	void OnClientInPlayerOneDied(const ClientInPlayerOneDiedEvent& event);
	void OnPlayerTwoDied(const std::string& author, const std::string& fraction);
	void OnClientInPlayerTwoDied(const ClientInPlayerTwoDiedEvent& event);
	void OnTankDied(const StatisticsTankDiedEvent& event);
	void OnBrickWallDied(const StatisticsAttributionEvent& event);
	void OnHostBrickWallDied(const BrickWallDiedEvent& event);
	void OnClientInBrickWallDied(const ClientInBrickWallDiedEvent& event);
	void OnSteelWallDied(const StatisticsAttributionEvent& event);
	void OnHostSteelWallDied(const SteelWallDiedEvent& event);
	void OnClientInSteelWallDied(const ClientInSteelWallDiedEvent& event);
	void OnBonusPickup(const StatisticsBonusPickupEvent& event);
	void OnClientInBonusPickup(const ClientInBonusPickupEvent& event);
	void OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event);
	void OnClientInBonusDestroyed(const ClientInBonusDestroyedEvent& event);

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
};
