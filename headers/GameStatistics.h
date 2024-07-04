#pragma once

#include "EventSystem.h"

#include <memory>
#include <string>

class GameStatistics
{
	std::string _name;
	std::shared_ptr<EventSystem> _events;

	// TODO: use std::atomic when multithreading is used
	int _enemyRespawnResource{20};
	int _playerOneRespawnResource{3};
	int _playerTwoRespawnResource{3};
	bool _enemyOneNeedRespawn{false};
	bool _enemyTwoNeedRespawn{false};
	bool _enemyThreeNeedRespawn{false};
	bool _enemyFourNeedRespawn{false};
	bool _playerOneNeedRespawn{false};
	bool _playerTwoNeedRespawn{false};
	bool _coopOneAINeedRespawn{false};
	bool _coopTwoAINeedRespawn{false};

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

	int _brickDiedByEnemyTeam{0};
	int _brickDiedByPlayerOne{0};
	int _brickDiedByPlayerTwo{0};

public:
	GameStatistics(std::shared_ptr<EventSystem> events);

	~GameStatistics();

	void Subscribe();
	void Unsubscribe() const;

	void BulletHit(const std::string& authorTeam);
	void EnemyHit(const std::string& authorTeam);
	void PlayerOneHit(const std::string& authorTeam);
	void PlayerTwoHit(const std::string& authorTeam);
	void EnemyDied(const std::string& authorTeam);
	void PlayerDied(const std::string& authorTeam);
	void BrickDied(const std::string& authorTeam);

	void Reset();

	[[nodiscard]] int GetEnemyRespawnResource() const { return _enemyRespawnResource; }
	[[nodiscard]] int GetPlayerOneRespawnResource() const { return _playerOneRespawnResource; }
	[[nodiscard]] int GetPlayerTwoRespawnResource() const { return _playerTwoRespawnResource; }
	[[nodiscard]] bool IsEnemyOneNeedRespawn() const { return _enemyOneNeedRespawn; }
	[[nodiscard]] bool IsEnemyTwoNeedRespawn() const { return _enemyTwoNeedRespawn; }
	[[nodiscard]] bool IsEnemyThreeNeedRespawn() const { return _enemyThreeNeedRespawn; }
	[[nodiscard]] bool IsEnemyFourNeedRespawn() const { return _enemyFourNeedRespawn; }
	[[nodiscard]] bool IsPlayerOneNeedRespawn() const { return _playerOneNeedRespawn; }
	[[nodiscard]] bool IsPlayerTwoNeedRespawn() const { return _playerTwoNeedRespawn; }
	[[nodiscard]] bool IsCoopOneAINeedRespawn() const { return _coopOneAINeedRespawn; }
	[[nodiscard]] bool IsCoopTwoAINeedRespawn() const { return _coopTwoAINeedRespawn; }

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

	[[nodiscard]] int GetBrickDiedByEnemyTeam() const { return _brickDiedByEnemyTeam; }
	[[nodiscard]] int GetBrickDiedByPlayerOne() const { return _brickDiedByPlayerOne; }
	[[nodiscard]] int GetBrickDiedByPlayerTwo() const { return _brickDiedByPlayerTwo; }
};
