#include "components/GameStatistics.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "enums/Faction.h"

GameStatistics::GameStatistics(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

void GameStatistics::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnGameReset));

	//NOTE: one set for both roles, and no role check anywhere below - the host reaches these off
	//its own game logic, the client off Client.cpp re-emitting the very same events. Replication
	//is not this class's business: Server subscribes to these same events and lives only on the host.
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
	if (event.faction == Faction::EnemyTeam)
	{
		++_data.bulletHitByEnemy;
	}
	else if (event.faction == Faction::PlayerTeam)
	{
		if (event.author.ends_with("1"))
		{
			++_data.bulletHitByPlayerOne;
		}
		else if (event.author.ends_with("2"))
		{
			++_data.bulletHitByPlayerTwo;
		}
	}
}

void GameStatistics::OnEnemyHit(const std::string& author, Faction faction)
{
	if (faction == Faction::EnemyTeam)
	{
		++_data.enemyHitByFriendlyFire;
	}
	else if (faction == Faction::PlayerTeam)
	{
		if (author.ends_with("1"))
		{
			++_data.enemyHitByPlayerOne;
		}
		else if (author.ends_with("2"))
		{
			++_data.enemyHitByPlayerTwo;
		}
	}
}

void GameStatistics::OnPlayerOneHit(const std::string& author, Faction faction)
{
	if (faction == Faction::EnemyTeam)
	{
		++_data.playerOneHitByEnemyTeam;
	}
	else if (faction == Faction::PlayerTeam)
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_data.playerOneHitFriendlyFire;
		}
	}
}

void GameStatistics::OnPlayerTwoHit(const std::string& author, Faction faction)
{
	if (faction == Faction::EnemyTeam)
	{
		++_data.playerTwoHitByEnemyTeam;
	}
	else if (faction == Faction::PlayerTeam)
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_data.playerTwoHitFriendlyFire;
		}
	}
}

void GameStatistics::OnTankHit(const StatisticsTankHitEvent& event)
{
	if (event.who.starts_with("Enemy"))
	{
		OnEnemyHit(event.author, event.faction);
	}
	else if (event.who.ends_with("1"))
	{
		OnPlayerOneHit(event.author, event.faction);
	}
	else if (event.who.ends_with("2"))
	{
		OnPlayerTwoHit(event.author, event.faction);
	}
}

void GameStatistics::OnEnemyDied(const std::string& author, Faction faction)
{
	if (faction == Faction::EnemyTeam)
	{
		++_data.enemyDiedByFriendlyFire;
	}
	else if (faction == Faction::PlayerTeam)
	{
		if (author.ends_with("1"))
		{
			++_data.enemyDiedByPlayerOne;
		}
		else if (author.ends_with("2"))
		{
			++_data.enemyDiedByPlayerTwo;
		}
	}
}

void GameStatistics::OnPlayerOneDied(const std::string& author, Faction faction)
{
	if (faction == Faction::EnemyTeam)
	{
		++_data.playerDiedByEnemyTeam;
	}
	else if (faction == Faction::PlayerTeam)
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_data.playerOneDiedByFriendlyFire;
		}
	}
}

void GameStatistics::OnPlayerTwoDied(const std::string& author, Faction faction)
{
	if (faction == Faction::EnemyTeam)
	{
		++_data.playerDiedByEnemyTeam;
	}
	else if (faction == Faction::PlayerTeam)
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_data.playerTwoDiedByFriendlyFire;
		}
	}
}

void GameStatistics::OnTankDied(const TankDiedEvent& event)
{
	if (event.who.starts_with("Enemy"))
	{
		OnEnemyDied(event.author, event.faction);
	}
	else if (event.who.ends_with("1"))
	{
		OnPlayerOneDied(event.author, event.faction);
	}
	else if (event.who.ends_with("2"))
	{
		OnPlayerTwoDied(event.author, event.faction);
	}
}

void GameStatistics::OnBrickWallDied(const BrickWallDiedEvent& event)
{
	if (event.faction == Faction::EnemyTeam)
	{
		++_data.brickWallDiedByEnemyTeam;
	}
	else if (event.faction == Faction::PlayerTeam)
	{
		if (event.author.ends_with("1"))
		{
			++_data.brickWallDiedByPlayerOne;
		}
		else if (event.author.ends_with("2"))
		{
			++_data.brickWallDiedByPlayerTwo;
		}
	}
}

void GameStatistics::OnSteelWallDied(const SteelWallDiedEvent& event)
{
	if (event.faction == Faction::EnemyTeam)
	{
		++_data.steelWallDiedByEnemyTeam;
	}
	else if (event.faction == Faction::PlayerTeam)
	{
		if (event.author.ends_with("1"))
		{
			++_data.steelWallDiedByPlayerOne;
		}
		else if (event.author.ends_with("2"))
		{
			++_data.steelWallDiedByPlayerTwo;
		}
	}
}

void GameStatistics::OnBonusPickup(const StatisticsBonusPickupEvent& event)
{
	if (event.faction == Faction::EnemyTeam)
	{
		++_data.bonusPickupByEnemyTeam;
	}
	else if (event.faction == Faction::PlayerTeam)
	{
		if (event.author.ends_with("1"))
		{
			++_data.bonusPickupByPlayerOne;
		}
		else if (event.author.ends_with("2"))
		{
			++_data.bonusPickupByPlayerTwo;
		}
	}
}

void GameStatistics::OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event)
{
	if (event.faction == Faction::EnemyTeam)
	{
		++_data.bonusDestroyedByEnemyTeam;
	}
	else if (event.faction == Faction::PlayerTeam)
	{
		if (event.author.ends_with("1"))
		{
			++_data.bonusDestroyedByPlayerOne;
		}
		else if (event.author.ends_with("2"))
		{
			++_data.bonusDestroyedByPlayerTwo;
		}
	}
}

void GameStatistics::OnBonusExpired(const StatisticsBonusExpiredEvent&)
{
	++_data.bonusExpired;
}

void GameStatistics::Reset() { _data = {}; }
