#pragma once

#include "EventSystem.h"

#include <memory>
#include <string>

class GameStatistics
{
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

	// TODO: use std::atomic when multithreading is used
	int enemyRespawnResource{20};
	int playerOneRespawnResource{3};
	int playerTwoRespawnResource{3};
	bool enemyOneNeedRespawn{false};
	bool enemyTwoNeedRespawn{false};
	bool enemyThreeNeedRespawn{false};
	bool enemyFourNeedRespawn{false};
	bool playerOneNeedRespawn{false};
	bool playerTwoNeedRespawn{false};
	bool coopOneAINeedRespawn{false};
	bool coopTwoAINeedRespawn{false};

	int bulletHitByEnemy{0};
	int bulletHitByPlayerOne{0};
	int bulletHitByPlayerTwo{0};

	int enemyHitByFriendlyFire{0};
	int enemyHitByPlayerOne{0};
	int enemyHitByPlayerTwo{0};

	int playerOneHitFriendlyFire{0};
	int playerOneHitByEnemyTeam{0};

	int playerTwoHitFriendlyFire{0};
	int playerTwoHitByEnemyTeam{0};

	int enemyDiedByFriendlyFire{0};
	int enemyDiedByPlayerOne{0};
	int enemyDiedByPlayerTwo{0};

	int playerOneDiedByFriendlyFire{0};
	int playerTwoDiedByFriendlyFire{0};
	int playerDiedByEnemyTeam{0};

	int brickDiedByEnemyTeam{0};
	int brickDiedByPlayerOne{0};
	int brickDiedByPlayerTwo{0};

private:
	std::string _name;
	std::shared_ptr<EventSystem> _events;
};
