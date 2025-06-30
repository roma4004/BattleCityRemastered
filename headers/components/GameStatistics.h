#pragma once

#include <memory>
#include <string>

enum class GameMode : char8_t;
class EventSystem;

class GameStatistics final
{
	//TODO: fix desync in statistics
	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};

	// TODO: use std::atomic when multithreading is used
	int _bulletHitByEnemy{0};
	int _bulletHitByPlayerOne{0};
	int _bulletHitByPlayerTwo{0};

	int _enemyHitByFriendlyFire{0};
	int _enemyHitByPlayerOne{0};
	int _enemyHitByPlayerTwo{0};

	int _playerOneHitFriendlyFire{0};
	int _playerOneHitByEnemyTeam{0};

	int _playerTwoHitFriendlyFire{0};
	int _playerTwoHitByEnemyTeam{0};

	int _enemyDiedByFriendlyFire{0};
	int _enemyDiedByPlayerOne{0};
	int _enemyDiedByPlayerTwo{0};

	int _playerOneDiedByFriendlyFire{0};
	int _playerTwoDiedByFriendlyFire{0};
	int _playerDiedByEnemyTeam{0};

	int _brickWallDiedByEnemyTeam{0};
	int _brickWallDiedByPlayerOne{0};
	int _brickWallDiedByPlayerTwo{0};

	int _steelWallDiedByEnemyTeam{0};
	int _steelWallDiedByPlayerOne{0};
	int _steelWallDiedByPlayerTwo{0};

	GameMode _gameMode{};

	void Subscribe();
	void SubscribeHost();
	void SubscribeAsClient();
	void OnGameModeChangedTo(GameMode newGameMode);
	void OnClientStatisticsChange(const std::string& type, const std::string& author, const std::string& fraction);

	void Unsubscribe() const;
	void UnsubscribeAsHost() const;
	void UnsubscribeAsClient() const;

	void OnBulletHit(const std::string& author, const std::string& fraction);
	void OnEnemyHit(const std::string& author, const std::string& fraction);
	void OnPlayerOneHit(const std::string& author, const std::string& fraction);
	void OnPlayerTwoHit(const std::string& author, const std::string& fraction);
	void OnTankHit(const std::string& who, const std::string& author, const std::string& fraction);
	void OnEnemyDied(const std::string& author, const std::string& fraction);
	void OnPlayerOneDied(const std::string& author, const std::string& fraction);
	void OnPlayerTwoDied(const std::string& author, const std::string& fraction);
	void OnTankDied(const std::string& who, const std::string& author, const std::string& fraction);
	void OnBrickWallDied(const std::string& author, const std::string& fraction);
	void OnSteelWallDied(const std::string& author, const std::string& fraction);

public:
	explicit GameStatistics(std::shared_ptr<EventSystem> events);

	~GameStatistics();

	void Reset();

	[[nodiscard]] int GetBulletHitByEnemy() const { return _bulletHitByEnemy; }
	[[nodiscard]] int GetBulletHitByPlayerOne() const { return _bulletHitByPlayerOne; }
	[[nodiscard]] int GetBulletHitByPlayerTwo() const { return _bulletHitByPlayerTwo; }

	[[nodiscard]] int GetEnemyHitByFriendlyFire() const { return _enemyHitByFriendlyFire; }
	[[nodiscard]] int GetEnemyHitByPlayerOne() const { return _enemyHitByPlayerOne; }
	[[nodiscard]] int GetEnemyHitByPlayerTwo() const { return _enemyHitByPlayerTwo; }

	[[nodiscard]] int GetPlayerOneHitFriendlyFire() const { return _playerOneHitFriendlyFire; }
	[[nodiscard]] int GetPlayerOneHitByEnemyTeam() const { return _playerOneHitByEnemyTeam; }

	[[nodiscard]] int GetPlayerTwoHitFriendlyFire() const { return _playerTwoHitFriendlyFire; }
	[[nodiscard]] int GetPlayerTwoHitByEnemyTeam() const { return _playerTwoHitByEnemyTeam; }

	[[nodiscard]] int GetEnemyDiedByFriendlyFire() const { return _enemyDiedByFriendlyFire; }
	[[nodiscard]] int GetEnemyDiedByPlayerOne() const { return _enemyDiedByPlayerOne; }
	[[nodiscard]] int GetEnemyDiedByPlayerTwo() const { return _enemyDiedByPlayerTwo; }

	[[nodiscard]] int GetPlayerOneDiedByFriendlyFire() const { return _playerOneDiedByFriendlyFire; }
	[[nodiscard]] int GetPlayerTwoDiedByFriendlyFire() const { return _playerTwoDiedByFriendlyFire; }
	[[nodiscard]] int GetPlayerDiedByEnemyTeam() const { return _playerDiedByEnemyTeam; }

	[[nodiscard]] int GetBrickWallDiedByEnemyTeam() const { return _brickWallDiedByEnemyTeam; }
	[[nodiscard]] int GetBrickWallDiedByPlayerOne() const { return _brickWallDiedByPlayerOne; }
	[[nodiscard]] int GetBrickWallDiedByPlayerTwo() const { return _brickWallDiedByPlayerTwo; }

	[[nodiscard]] int GetSteelWallDiedByEnemyTeam() const { return _steelWallDiedByEnemyTeam; }
	[[nodiscard]] int GetSteelWallDiedByPlayerOne() const { return _steelWallDiedByPlayerOne; }
	[[nodiscard]] int GetSteelWallDiedByPlayerTwo() const { return _steelWallDiedByPlayerTwo; }
};
