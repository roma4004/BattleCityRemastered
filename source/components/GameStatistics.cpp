#include "components/GameStatistics.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "enums/Author.h"

namespace
{
//NOTE: every counter here splits the same three ways
struct Buckets final
{
	unsigned short& byEnemyTeam;
	unsigned short& byPlayerOne;
	unsigned short& byPlayerTwo;
};

void Credit(const Author author, const Buckets buckets)
{
	switch (author)
	{
		case Author::Enemy1:
		case Author::Enemy2:
		case Author::Enemy3:
		case Author::Enemy4:
			++buckets.byEnemyTeam;
			break;
		case Author::Player1:
			++buckets.byPlayerOne;
			break;
		case Author::Player2:
			++buckets.byPlayerTwo;
			break;
		case Author::None:
		case Author::lastId:
			break;
	}
}
}//namespace

GameStatistics::GameStatistics(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

void GameStatistics::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnGameReset));

	//NOTE: one set for both roles - the host reaches these off its own game logic, the client off the
	//same events re-emitted from the wire, so nothing below asks which one it is
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBulletHit));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnTankHit));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnTankDied));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBrickWallDied));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnSteelWallDied));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBonusPickup));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBonusDestroyed));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnBonusExpired));
}

void GameStatistics::OnGameReset(const GameResetEvent&) { Reset(); }

void GameStatistics::OnBulletHit(const StatisticsBulletHitEvent& event)
{
	Credit(event.author, {.byEnemyTeam = _data.bulletHitByEnemy,
						  .byPlayerOne = _data.bulletHitByPlayerOne,
						  .byPlayerTwo = _data.bulletHitByPlayerTwo});
}

//NOTE: friendly fire is one bucket, whichever player pulled the trigger - hence the same
//counter in two positions
void GameStatistics::OnTankHit(const StatisticsTankHitEvent& event)
{
	switch (event.who)
	{
		case Author::Enemy1:
		case Author::Enemy2:
		case Author::Enemy3:
		case Author::Enemy4:
			Credit(event.author, {.byEnemyTeam = _data.enemyHitByFriendlyFire,
								  .byPlayerOne = _data.enemyHitByPlayerOne,
								  .byPlayerTwo = _data.enemyHitByPlayerTwo});
			break;
		case Author::Player1:
			Credit(event.author, {.byEnemyTeam = _data.playerOneHitByEnemyTeam,
								  .byPlayerOne = _data.playerOneHitFriendlyFire,
								  .byPlayerTwo = _data.playerOneHitFriendlyFire});
			break;
		case Author::Player2:
			Credit(event.author, {.byEnemyTeam = _data.playerTwoHitByEnemyTeam,
								  .byPlayerOne = _data.playerTwoHitFriendlyFire,
								  .byPlayerTwo = _data.playerTwoHitFriendlyFire});
			break;
		case Author::None:
		case Author::lastId:
			break;
	}
}

void GameStatistics::OnTankDied(const TankDiedEvent& event)
{
	switch (event.who)
	{
		case Author::Enemy1:
		case Author::Enemy2:
		case Author::Enemy3:
		case Author::Enemy4:
			Credit(event.author, {.byEnemyTeam = _data.enemyDiedByFriendlyFire,
								  .byPlayerOne = _data.enemyDiedByPlayerOne,
								  .byPlayerTwo = _data.enemyDiedByPlayerTwo});
			break;
		case Author::Player1:
			Credit(event.author, {.byEnemyTeam = _data.playerDiedByEnemyTeam,
								  .byPlayerOne = _data.playerOneDiedByFriendlyFire,
								  .byPlayerTwo = _data.playerOneDiedByFriendlyFire});
			break;
		case Author::Player2:
			Credit(event.author, {.byEnemyTeam = _data.playerDiedByEnemyTeam,
								  .byPlayerOne = _data.playerTwoDiedByFriendlyFire,
								  .byPlayerTwo = _data.playerTwoDiedByFriendlyFire});
			break;
		case Author::None:
		case Author::lastId:
			break;
	}
}

void GameStatistics::OnBrickWallDied(const BrickWallDiedEvent& event)
{
	Credit(event.author, {.byEnemyTeam = _data.brickWallDiedByEnemyTeam,
						  .byPlayerOne = _data.brickWallDiedByPlayerOne,
						  .byPlayerTwo = _data.brickWallDiedByPlayerTwo});
}

void GameStatistics::OnSteelWallDied(const SteelWallDiedEvent& event)
{
	Credit(event.author, {.byEnemyTeam = _data.steelWallDiedByEnemyTeam,
						  .byPlayerOne = _data.steelWallDiedByPlayerOne,
						  .byPlayerTwo = _data.steelWallDiedByPlayerTwo});
}

void GameStatistics::OnBonusPickup(const StatisticsBonusPickupEvent& event)
{
	Credit(event.author, {.byEnemyTeam = _data.bonusPickupByEnemyTeam,
						  .byPlayerOne = _data.bonusPickupByPlayerOne,
						  .byPlayerTwo = _data.bonusPickupByPlayerTwo});
}

void GameStatistics::OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event)
{
	Credit(event.author, {.byEnemyTeam = _data.bonusDestroyedByEnemyTeam,
						  .byPlayerOne = _data.bonusDestroyedByPlayerOne,
						  .byPlayerTwo = _data.bonusDestroyedByPlayerTwo});
}

void GameStatistics::OnBonusExpired(const StatisticsBonusExpiredEvent&)
{
	++_data.bonusExpired;
}

void GameStatistics::Reset() { _data = {}; }
