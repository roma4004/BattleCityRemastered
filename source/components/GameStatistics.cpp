#include "components/GameStatistics.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"

GameStatistics::GameStatistics(const std::shared_ptr<EventSystem>& events)
	: _name{"Statistics"}
	, _events{events}
{
	Subscribe();
}

GameStatistics::~GameStatistics()
{
	Unsubscribe();
}

void GameStatistics::Subscribe()
{
	_events->AddListener("Reset", _name, [this]() { Reset(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->OnGameModeChangedTo(newGameMode);
	});

	SubscribeHost();
}

void GameStatistics::SubscribeHost()
{
	//TODO: replace <std::string> with <Enum::statisticsType>
	_events->AddListener("Statistics_BulletHit", _name, [this](const StatisticsAttributionEvent& event)
	{
		this->OnBulletHit(event);
	});

	_events->AddListener(
			"Statistics_TankHit", _name,
			[this](const TankStatisticsEvent& event)
			{
				this->OnTankHit(event);
			});

	_events->AddListener(
			"Statistics_TankDied", _name,
			[this](const TankStatisticsEvent& event)
			{
				this->OnTankDied(event);
			});

	_events->AddListener(
			"Statistics_BrickWallDied", _name,
			[this](const StatisticsAttributionEvent& event)
			{
				OnBrickWallDied(event);
			});

	_events->AddListener(
			"Statistics_SteelWallDied", _name,
			[this](const StatisticsAttributionEvent& event)
			{
				OnSteelWallDied(event);
			});

	_events->AddListener(
			"Statistics_BonusPickup", _name,
			[this](const StatisticsAttributionEvent& event)
			{
				OnBonusPickup(event);
			});

	_events->AddListener(
			"Statistics_BonusDestroyed", _name,
			[this](const StatisticsAttributionEvent& event)
			{
				OnBonusDestroyed(event);
			});
}

void GameStatistics::SubscribeAsClient()
{
	_events->AddListener(
			"ClientReceived_Statistics", _name,
			[this](const ClientReceivedStatisticsEvent& event)
			{
				this->OnClientStatisticsChange(event);
			});
}

void GameStatistics::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void GameStatistics::UnsubscribeAsHost() const
{
	_events->RemoveListener("Statistics_BulletHit", _name);
	_events->RemoveListener("Statistics_TankHit", _name);
	_events->RemoveListener("Statistics_TankDied", _name);

	_events->RemoveListener("Statistics_BrickWallDied", _name);
	_events->RemoveListener("Statistics_SteelWallDied", _name);
	_events->RemoveListener("Statistics_BonusPickup", _name);
}

void GameStatistics::UnsubscribeAsClient() const { _events->RemoveListener("ClientReceived_Statistics", _name); }

void GameStatistics::OnGameModeChangedTo(const GameMode newGameMode)
{
	this->_gameMode = newGameMode;

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

void GameStatistics::OnClientStatisticsChange(const ClientReceivedStatisticsEvent& event)
{
	const std::string& type = event.eventName;
	const std::string& author = event.author;
	const std::string& fraction = event.fraction;

	if (type == "BulletHit")
	{
		OnBulletHit(StatisticsAttributionEvent{author, fraction});
	}
	else if (type == "EnemyHit")
	{
		OnEnemyHit(author, fraction);
	}
	else if (type == "PlayerOneHit")
	{
		OnPlayerOneHit(author, fraction);
	}
	else if (type == "PlayerTwoHit")
	{
		OnPlayerTwoHit(author, fraction);
	}
	else if (type == "EnemyDied")
	{
		OnEnemyDied(author, fraction);
	}
	else if (type == "PlayerOneDied")
	{
		OnPlayerOneDied(author, fraction);
	}
	else if (type == "PlayerTwoDied")
	{
		OnPlayerTwoDied(author, fraction);
	}
	else if (type == "BrickWallDied")
	{
		OnBrickWallDied(StatisticsAttributionEvent{author, fraction});
	}
	else if (type == "SteelWallDied")
	{
		OnSteelWallDied(StatisticsAttributionEvent{author, fraction});
	}
	else if (type == "BonusPickup")
	{
		OnBonusPickup(StatisticsAttributionEvent{author, fraction});
	}
	else if (type == "BonusDestroyed")
	{
		OnBonusDestroyed(StatisticsAttributionEvent{author, fraction});
	}
}

void GameStatistics::OnBulletHit(const StatisticsAttributionEvent& event)
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"BulletHit", event.author, event.fraction});
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"EnemyHit", author, fraction});
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"PlayerOneHit", author, fraction});
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"PlayerTwoHit", author, fraction});
	}
}

void GameStatistics::OnTankHit(const TankStatisticsEvent& event)
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"EnemyDied", author, fraction});
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"PlayerOneDied", author, fraction});
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"PlayerTwoDied", author, fraction});
	}
}

void GameStatistics::OnTankDied(const TankStatisticsEvent& event)
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"BrickWallDied", event.author, event.fraction});
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"SteelWallDied", event.author, event.fraction});
	}
}

void GameStatistics::OnBonusPickup(const StatisticsAttributionEvent& event)
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"BonusPickup", event.author, event.fraction});
	}
}

void GameStatistics::OnBonusDestroyed(const StatisticsAttributionEvent& event)
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
		_events->EmitEvent("ServerSend_Statistics", ServerSendStatisticsEvent{"BonusDestroyed", event.author, event.fraction});
	}
}

void GameStatistics::Reset() { _data = {}; }
