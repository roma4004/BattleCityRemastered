#include "components/GameStatistics.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"

//TODO: write statistics for pickuped bonuses
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
			[this](const std::string& author, const std::string& fraction)
			{
				OnBrickWallDied(author, fraction);
			});

	_events->AddListener(
			"Statistics_SteelWallDied", _name,
			[this](const std::string& author, const std::string& fraction)
			{
				OnSteelWallDied(author, fraction);
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

void GameStatistics::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);

	_gameMode == GameMode::PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();
}

void GameStatistics::UnsubscribeAsHost() const
{
	_events->RemoveListener("Statistics_BulletHit", _name);
	_events->RemoveListener("Statistics_TankHit", _name);
	_events->RemoveListener("Statistics_TankDied", _name);

	_events->RemoveListener("Statistics_BrickWallDied", _name);
	_events->RemoveListener("Statistics_SteelWallDied", _name);
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
		OnBrickWallDied(author, fraction);
	}
	else if (type == "SteelWallDied")
	{
		OnSteelWallDied(author, fraction);
	}
}

void GameStatistics::OnBulletHit(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_bulletHitByEnemy;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1"))
		{
			++_bulletHitByPlayerOne;
		}
		else if (author.ends_with("2"))
		{
			++_bulletHitByPlayerTwo;
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
		++_enemyHitByFriendlyFire;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1"))
		{
			++_enemyHitByPlayerOne;
		}
		else if (author.ends_with("2"))
		{
			++_enemyHitByPlayerTwo;
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
		++_playerOneHitByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_playerOneHitFriendlyFire;
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
		++_playerTwoHitByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_playerTwoHitFriendlyFire;
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
		++_enemyDiedByFriendlyFire;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1"))
		{
			++_enemyDiedByPlayerOne;
		}
		else if (author.ends_with("2"))
		{
			++_enemyDiedByPlayerTwo;
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
		++_playerDiedByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_playerOneDiedByFriendlyFire;
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
		++_playerDiedByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1") || author.ends_with("2"))
		{
			++_playerTwoDiedByFriendlyFire;
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

void GameStatistics::OnBrickWallDied(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_brickWallDiedByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1"))
		{
			++_brickWallDiedByPlayerOne;
		}
		else if (author.ends_with("2"))
		{
			++_brickWallDiedByPlayerTwo;
		}
	}

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_Statistics", "BrickWallDied", author, fraction);
	}
}

void GameStatistics::OnSteelWallDied(const std::string& author, const std::string& fraction)
{
	if (fraction.starts_with("Enemy"))
	{
		++_steelWallDiedByEnemyTeam;
	}
	else if (fraction.starts_with("Player"))
	{
		if (author.ends_with("1"))
		{
			++_steelWallDiedByPlayerOne;
		}
		else if (author.ends_with("2"))
		{
			++_steelWallDiedByPlayerTwo;
		}
	}

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_Statistics", "SteelWallDied", author, fraction);
	}
}

void GameStatistics::Reset()
{
	_bulletHitByEnemy = 0;
	_bulletHitByPlayerOne = 0;
	_bulletHitByPlayerTwo = 0;

	_enemyHitByFriendlyFire = 0;
	_enemyHitByPlayerOne = 0;
	_enemyHitByPlayerTwo = 0;

	_playerOneHitFriendlyFire = 0;
	_playerOneHitByEnemyTeam = 0;

	_playerTwoHitFriendlyFire = 0;
	_playerTwoHitByEnemyTeam = 0;

	_enemyDiedByFriendlyFire = 0;
	_enemyDiedByPlayerOne = 0;
	_enemyDiedByPlayerTwo = 0;

	_playerOneDiedByFriendlyFire = 0;
	_playerTwoDiedByFriendlyFire = 0;
	_playerDiedByEnemyTeam = 0;

	_brickWallDiedByEnemyTeam = 0;
	_brickWallDiedByPlayerOne = 0;
	_brickWallDiedByPlayerTwo = 0;
}
