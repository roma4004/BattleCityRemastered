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
	_events->AddListener("Statistics_BulletHit", _name, [this](const std::string& author, const std::string& fraction)
	{
		this->OnBulletHit(author, fraction);
	});

	_events->AddListener(
			"Statistics_TankHit", _name,
			[this](const std::string& whoHit, const std::string& author, const std::string& fraction)
			{
				this->OnTankHit(whoHit, author, fraction);
			});

	_events->AddListener(
			"Statistics_TankDied", _name,
			[this](const std::string& whoDied, const std::string& author, const std::string& fraction)
			{
				this->OnTankDied(whoDied, author, fraction);
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
			[this](const std::string& type, const std::string& author, const std::string& fraction)
			{
				this->OnClientStatisticsChange(type, author, fraction);
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

void GameStatistics::OnClientStatisticsChange(const std::string& type, const std::string& author,
											  const std::string& fraction)
{
	if (type == "BulletHit")
	{
		OnBulletHit(author, fraction);
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

void GameStatistics::OnBulletHit(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_data.bulletHitByEnemy;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1"))
		{
			++_data.bulletHitByPlayerOne;
		}
		else if (author.ends_with("2"))
		{
			++_data.bulletHitByPlayerTwo;
		}
	}

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_Statistics", "BulletHit", author, fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "EnemyHit", author, fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "PlayerOneHit", author, fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "PlayerTwoHit", author, fraction);
	}
}

void GameStatistics::OnTankHit(const std::string& who, const std::string& author, const std::string& fraction)
{
	if (who.starts_with("Enemy"))
	{
		OnEnemyHit(author, fraction);
	}
	else if (who.ends_with("1"))
	{
		OnPlayerOneHit(author, fraction);
	}
	else if (who.ends_with("2"))
	{
		OnPlayerTwoHit(author, fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "EnemyDied", author, fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "PlayerOneDied", author, fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "PlayerTwoDied", author, fraction);
	}
}

void GameStatistics::OnTankDied(const std::string& who, const std::string& author, const std::string& fraction)
{
	if (who.starts_with("Enemy"))
	{
		OnEnemyDied(author, fraction);
	}
	else if (who.ends_with("1"))
	{
		OnPlayerOneDied(author, fraction);
	}
	else if (who.ends_with("2"))
	{
		OnPlayerTwoDied(author, fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "BrickWallDied", event.author, event.fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "SteelWallDied", event.author, event.fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "BonusPickup", event.author, event.fraction);
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
		_events->EmitEvent("ServerSend_Statistics", "BonusDestroyed", event.author, event.fraction);
	}
}

void GameStatistics::Reset() { _data = {}; }
