#include "components/GameStatistics.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "enums/GameMode.h"

GameStatistics::GameStatistics(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

void GameStatistics::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnGameReset));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnGameModeChangedTo));

	SubscribeAsAuthority();
}

void GameStatistics::OnGameReset(const GameResetEvent&) { Reset(); }

void GameStatistics::SubscribeAsAuthority()
{
	_authoritySubs.push_back(_events->AddListener(this, &GameStatistics::OnBulletHit));
	_authoritySubs.push_back(_events->AddListener(this, &GameStatistics::OnTankHit));
	_authoritySubs.push_back(_events->AddListener(this, &GameStatistics::OnTankDied));

	//NOTE: emit side uses BrickWallDiedEvent/SteelWallDiedEvent (see Obstacle.cpp/FortressWall.cpp);
	//OnBrickWallDied/OnSteelWallDied themselves still take StatisticsAttributionEvent as an
	//adapter type.
	_authoritySubs.push_back(_events->AddListener(this, &GameStatistics::OnAuthorityBrickWallDied));
	_authoritySubs.push_back(_events->AddListener(this, &GameStatistics::OnAuthoritySteelWallDied));
	_authoritySubs.push_back(_events->AddListener(this, &GameStatistics::OnBonusPickup));
	_authoritySubs.push_back(_events->AddListener(this, &GameStatistics::OnBonusDestroyed));
}

void GameStatistics::OnAuthorityBrickWallDied(const BrickWallDiedEvent& event)
{
	OnBrickWallDied(StatisticsAttributionEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnAuthoritySteelWallDied(const SteelWallDiedEvent& event)
{
	OnSteelWallDied(StatisticsAttributionEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::SubscribeAsClient()
{
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInBulletHit));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInEnemyHit));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInPlayerOneHit));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInPlayerTwoHit));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInEnemyDied));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInPlayerOneDied));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInPlayerTwoDied));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInBrickWallDied));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInSteelWallDied));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInBonusPickup));
	_clientSubs.push_back(_events->AddListener(this, &GameStatistics::OnClientInBonusDestroyed));
}

void GameStatistics::OnClientInBulletHit(const ClientInBulletHitEvent& event)
{
	OnBulletHit(StatisticsBulletHitEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnClientInEnemyHit(const ClientInEnemyHitEvent& event)
{
	OnEnemyHit(event.author, event.fraction);
}

void GameStatistics::OnClientInPlayerOneHit(const ClientInPlayerOneHitEvent& event)
{
	OnPlayerOneHit(event.author, event.fraction);
}

void GameStatistics::OnClientInPlayerTwoHit(const ClientInPlayerTwoHitEvent& event)
{
	OnPlayerTwoHit(event.author, event.fraction);
}

void GameStatistics::OnClientInEnemyDied(const ClientInEnemyDiedEvent& event)
{
	OnEnemyDied(event.author, event.fraction);
}

void GameStatistics::OnClientInPlayerOneDied(const ClientInPlayerOneDiedEvent& event)
{
	OnPlayerOneDied(event.author, event.fraction);
}

void GameStatistics::OnClientInPlayerTwoDied(const ClientInPlayerTwoDiedEvent& event)
{
	OnPlayerTwoDied(event.author, event.fraction);
}

void GameStatistics::OnClientInBrickWallDied(const ClientInBrickWallDiedEvent& event)
{
	OnBrickWallDied(StatisticsAttributionEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnClientInSteelWallDied(const ClientInSteelWallDiedEvent& event)
{
	OnSteelWallDied(StatisticsAttributionEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnClientInBonusPickup(const ClientInBonusPickupEvent& event)
{
	OnBonusPickup(StatisticsBonusPickupEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnClientInBonusDestroyed(const ClientInBonusDestroyedEvent& event)
{
	OnBonusDestroyed(StatisticsBonusDestroyedEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::UnsubscribeAsAuthority() { _authoritySubs.clear(); }

void GameStatistics::UnsubscribeAsClient() { _clientSubs.clear(); }

void GameStatistics::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	_gameMode = event.mode;

	if (IsClient(_gameMode))
	{
		UnsubscribeAsAuthority();
		SubscribeAsClient();
	}
	else
	{
		UnsubscribeAsClient();
		SubscribeAsAuthority();
	}
}

void GameStatistics::OnBulletHit(const StatisticsBulletHitEvent& event)
{
	if (event.fraction.starts_with("Enemy"))
	{
		++_data.bulletHitByEnemy;
	}
	else if (event.fraction.starts_with("Player"))
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

	EmitReplicated(ServerOutBulletHitEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnEnemyHit(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_data.enemyHitByFriendlyFire;
	}
	else if (fraction.starts_with("Player"))
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

	EmitReplicated(ServerOutEnemyHitEvent{.author = author, .fraction = fraction});
}

void GameStatistics::OnPlayerOneHit(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_data.playerOneHitByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_data.playerOneHitFriendlyFire;
		}
	}

	EmitReplicated(ServerOutPlayerOneHitEvent{.author = author, .fraction = fraction});
}

void GameStatistics::OnPlayerTwoHit(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_data.playerTwoHitByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_data.playerTwoHitFriendlyFire;
		}
	}

	EmitReplicated(ServerOutPlayerTwoHitEvent{.author = author, .fraction = fraction});
}

void GameStatistics::OnTankHit(const StatisticsTankHitEvent& event)
{
	if (event.who.starts_with("Enemy"))
	{
		OnEnemyHit(event.author, event.fraction);
	}
	else if (event.who.ends_with("1"))
	{
		OnPlayerOneHit(event.author, event.fraction);
	}
	else if (event.who.ends_with("2"))
	{
		OnPlayerTwoHit(event.author, event.fraction);
	}
}

void GameStatistics::OnEnemyDied(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_data.enemyDiedByFriendlyFire;
	}
	else if (fraction.starts_with("Player"))
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

	EmitReplicated(ServerOutEnemyDiedEvent{.author = author, .fraction = fraction});
}

void GameStatistics::OnPlayerOneDied(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_data.playerDiedByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_data.playerOneDiedByFriendlyFire;
		}
	}

	EmitReplicated(ServerOutPlayerOneDiedEvent{.author = author, .fraction = fraction});
}

void GameStatistics::OnPlayerTwoDied(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_data.playerDiedByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_data.playerTwoDiedByFriendlyFire;
		}
	}

	EmitReplicated(ServerOutPlayerTwoDiedEvent{.author = author, .fraction = fraction});
}

void GameStatistics::OnTankDied(const StatisticsTankDiedEvent& event)
{
	if (event.who.starts_with("Enemy"))
	{
		OnEnemyDied(event.author, event.fraction);
	}
	else if (event.who.ends_with("1"))
	{
		OnPlayerOneDied(event.author, event.fraction);
	}
	else if (event.who.ends_with("2"))
	{
		OnPlayerTwoDied(event.author, event.fraction);
	}
}

void GameStatistics::OnBrickWallDied(const StatisticsAttributionEvent& event)
{
	if (event.fraction.starts_with("Enemy"))
	{
		++_data.brickWallDiedByEnemyTeam;
	}
	else if (event.fraction.starts_with("Player"))
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

	EmitReplicated(ServerOutBrickWallDiedEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnSteelWallDied(const StatisticsAttributionEvent& event)
{
	if (event.fraction.starts_with("Enemy"))
	{
		++_data.steelWallDiedByEnemyTeam;
	}
	else if (event.fraction.starts_with("Player"))
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

	EmitReplicated(ServerOutSteelWallDiedEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnBonusPickup(const StatisticsBonusPickupEvent& event)
{
	if (event.fraction.starts_with("Enemy"))
	{
		++_data.bonusPickupByEnemyTeam;
	}
	else if (event.fraction.starts_with("Player"))
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

	EmitReplicated(ServerOutBonusPickupEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event)
{
	if (event.fraction.starts_with("Enemy"))
	{
		++_data.bonusDestroyedByEnemyTeam;
	}
	else if (event.fraction.starts_with("Player"))
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

	EmitReplicated(ServerOutBonusDestroyedEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::Reset() { _data = {}; }
