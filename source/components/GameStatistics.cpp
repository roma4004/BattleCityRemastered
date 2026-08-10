#include "components/GameStatistics.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "enums/GameMode.h"

GameStatistics::GameStatistics(const std::shared_ptr<EventSystem>& events)
	: _name{"Statistics"}
	, _events{events}
{
	Subscribe();
}

void GameStatistics::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnGameReset));
	_subs.push_back(_events->AddListener(this, &GameStatistics::OnGameModeChangedTo));

	SubscribeHost();
}

void GameStatistics::OnGameReset(const GameResetEvent&) { Reset(); }

void GameStatistics::SubscribeHost()
{
	_hostSubs.push_back(_events->AddListener(this, &GameStatistics::OnBulletHit));
	_hostSubs.push_back(_events->AddListener(this, &GameStatistics::OnTankHit));
	_hostSubs.push_back(_events->AddListener(this, &GameStatistics::OnTankDied));

	//NOTE: emit side uses BrickWallDiedEvent/SteelWallDiedEvent (see Obstacle.cpp/FortressWall.cpp);
	//OnBrickWallDied/OnSteelWallDied themselves still take StatisticsAttributionEvent as an
	//adapter type.
	_hostSubs.push_back(_events->AddListener(this, &GameStatistics::OnHostBrickWallDied));
	_hostSubs.push_back(_events->AddListener(this, &GameStatistics::OnHostSteelWallDied));
	_hostSubs.push_back(_events->AddListener(this, &GameStatistics::OnBonusPickup));
	_hostSubs.push_back(_events->AddListener(this, &GameStatistics::OnBonusDestroyed));
}

void GameStatistics::OnHostBrickWallDied(const BrickWallDiedEvent& event)
{
	OnBrickWallDied(StatisticsAttributionEvent{.author = event.author, .fraction = event.fraction});
}

void GameStatistics::OnHostSteelWallDied(const SteelWallDiedEvent& event)
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

void GameStatistics::OnClientInEnemyHit(const ClientInEnemyHitEvent& event) { OnEnemyHit(event.author, event.fraction); }

void GameStatistics::OnClientInPlayerOneHit(const ClientInPlayerOneHitEvent& event)
{
	OnPlayerOneHit(event.author, event.fraction);
}

void GameStatistics::OnClientInPlayerTwoHit(const ClientInPlayerTwoHitEvent& event)
{
	OnPlayerTwoHit(event.author, event.fraction);
}

void GameStatistics::OnClientInEnemyDied(const ClientInEnemyDiedEvent& event) { OnEnemyDied(event.author, event.fraction); }

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

void GameStatistics::UnsubscribeAsHost() { _hostSubs.clear(); }

void GameStatistics::UnsubscribeAsClient() { _clientSubs.clear(); }

void GameStatistics::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	_gameMode = event.mode;

	if (_gameMode == GameMode::PlayAsClient)
	{
		UnsubscribeAsHost();
		SubscribeAsClient();
	}
	else
	{
		UnsubscribeAsClient();
		SubscribeHost();
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutBulletHitEvent{.author = event.author, .fraction = event.fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutEnemyHitEvent{.author = author, .fraction = fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutPlayerOneHitEvent{.author = author, .fraction = fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutPlayerTwoHitEvent{.author = author, .fraction = fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutEnemyDiedEvent{.author = author, .fraction = fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutPlayerOneDiedEvent{.author = author, .fraction = fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutPlayerTwoDiedEvent{.author = author, .fraction = fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutBrickWallDiedEvent{.author = event.author, .fraction = event.fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutSteelWallDiedEvent{.author = event.author, .fraction = event.fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutBonusPickupEvent{.author = event.author, .fraction = event.fraction});
	}
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutBonusDestroyedEvent{.author = event.author, .fraction = event.fraction});
	}
}

void GameStatistics::Reset() { _data = {}; }
