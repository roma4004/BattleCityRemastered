#pragma once

#include "GameMode.h"

#include <memory>
#include <string>

class EventSystem;

class GameStatistics final
{
	std::string _name;
	GameMode _gameMode{Demo};
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

public:
	explicit GameStatistics(std::shared_ptr<EventSystem> events);

	~GameStatistics();

	void Subscribe();
	void SubscribeHost();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsHost() const;
	void UnsubscribeAsClient() const;

	void BulletHit(const std::string& author, const std::string& fraction);
	void EnemyHit(const std::string& author, const std::string& fraction);
	void PlayerOneHit(const std::string& author, const std::string& fraction);
	void PlayerTwoHit(const std::string& author, const std::string& fraction);
	void EnemyDied(const std::string& author, const std::string& fraction);
	void PlayerOneDied(const std::string& author, const std::string& fraction);
	void PlayerTwoDied(const std::string& author, const std::string& fraction);
	void BrickWallDied(const std::string& author, const std::string& fraction);
	void SteelWallDied(const std::string& author, const std::string& fraction);

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
