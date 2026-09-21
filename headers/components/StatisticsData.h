#pragma once

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
