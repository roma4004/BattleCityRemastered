#include "../headers/GameStatistics.h"

GameStatistics::GameStatistics(std::shared_ptr<EventSystem> events)
	: _name{"Statistics"}, _events{std::move(events)}
{
	Reset();
	Subscribe();
}

GameStatistics::~GameStatistics()
{
	Unsubscribe();
}

void GameStatistics::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Enemy1_Died", _name, [this]() { _enemyOneNeedRespawn = true; });
	_events->AddListener("Enemy2_Died", _name, [this]() { _enemyTwoNeedRespawn = true; });
	_events->AddListener("Enemy3_Died", _name, [this]() { _enemyThreeNeedRespawn = true; });
	_events->AddListener("Enemy4_Died", _name, [this]() { _enemyFourNeedRespawn = true; });

	_events->AddListener("PlayerOne_Died", _name, [this]() { _playerOneNeedRespawn = true; });
	_events->AddListener("PlayerTwo_Died", _name, [this]() { _playerTwoNeedRespawn = true; });
	_events->AddListener("CoopOneAI_Died", _name, [this]() { _coopOneAINeedRespawn = true; });
	_events->AddListener("CoopTwoAI_Died", _name, [this]() { _coopTwoAINeedRespawn = true; });

	_events->AddListener("PlayerOne_Spawn", _name, [this]()
	{
		_playerOneNeedRespawn = false;
		--_playerOneRespawnResource;
	});
	_events->AddListener("PlayerTwo_Spawn", _name, [this]()
	{
		_playerTwoNeedRespawn = false;
		--_playerTwoRespawnResource;
	});
	_events->AddListener("CoopOneAI_Spawn", _name, [this]()
	{
		_coopOneAINeedRespawn = false;
		--_playerOneRespawnResource;
	});
	_events->AddListener("CoopTwoAI_Spawn", _name, [this]()
	{
		_coopTwoAINeedRespawn = false;
		--_playerTwoRespawnResource;
	});

	_events->AddListener("Enemy1_Spawn", _name, [this]()
	{
		_enemyOneNeedRespawn = false;
		--_enemyRespawnResource;
	});
	_events->AddListener("Enemy2_Spawn", _name, [this]()
	{
		_enemyTwoNeedRespawn = false;
		--_enemyRespawnResource;
	});
	_events->AddListener("Enemy3_Spawn", _name, [this]()
	{
		_enemyThreeNeedRespawn = false;
		--_enemyRespawnResource;
	});
	_events->AddListener("Enemy4_Spawn", _name, [this]()
	{
		_enemyFourNeedRespawn = false;
		--_enemyRespawnResource;
	});

	//TODO: replace <std::string> with <Enum::statisticsType>
	_events->AddListener<const std::string&, const std::string&>(
			"BulletHit",
			_name,
			[this](const std::string& author, const std::string& fraction) { BulletHit(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"EnemyHit",
			_name,
			[this](const std::string& author, const std::string& fraction) { EnemyHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"EnemyDied",
			_name,
			[this](const std::string& author, const std::string& fraction) { EnemyDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"PlayerOneHit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerOneHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"PlayerOneDied",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerOneDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"CoopOneAIHit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerOneHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"CoopOneAIDied",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerOneDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"PlayerTwoHit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerTwoHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"PlayerTwoDied",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerTwoDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"CoopTwoAIHit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerTwoHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"CoopTwoAIDied",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerTwoDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"BrickDied",
			_name,
			[this](const std::string& author, const std::string& fraction) { BrickDied(author, fraction); });
}

void GameStatistics::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Enemy1_Died", _name);
	_events->RemoveListener("Enemy2_Died", _name);
	_events->RemoveListener("Enemy3_Died", _name);
	_events->RemoveListener("Enemy4_Died", _name);

	_events->RemoveListener("PlayerOne_Died", _name);
	_events->RemoveListener("PlayerTwo_Died", _name);
	_events->RemoveListener("CoopOneAI_Died", _name);
	_events->RemoveListener("CoopTwoAI_Died", _name);

	_events->RemoveListener("PlayerOne_Spawn", _name);
	_events->RemoveListener("PlayerTwo_Spawn", _name);
	_events->RemoveListener("CoopOneAI_Spawn", _name);
	_events->RemoveListener("CoopTwoAI_Spawn", _name);

	_events->RemoveListener("Enemy1_Spawn", _name);
	_events->RemoveListener("Enemy2_Spawn", _name);
	_events->RemoveListener("Enemy3_Spawn", _name);
	_events->RemoveListener("Enemy4_Spawn", _name);

	_events->RemoveListener<const std::string&, const std::string&>("BulletHit", _name);

	_events->RemoveListener<const std::string&, const std::string&>("EnemyHit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("EnemyDied", _name);

	_events->RemoveListener<const std::string&, const std::string&>("PlayerOneHit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("PlayerOneDied", _name);

	_events->RemoveListener<const std::string&, const std::string&>("CoopOneAIHit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("CoopOneAIDied", _name);

	_events->RemoveListener<const std::string&, const std::string&>("PlayerTwoHit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("PlayerTwoDied", _name);

	_events->RemoveListener<const std::string&, const std::string&>("CoopTwoAIHit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("CoopTwoAIDied", _name);

	_events->RemoveListener<const std::string&, const std::string&>("BrickDied", _name);
}

void GameStatistics::BulletHit(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		if (author.compare(0, 4, "Enemy"))
		{
			++_bulletHitByEnemy;
		}
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "PlayerOne" || author == "CoopOneAI")
		{
			++_bulletHitByPlayerOne;
		}
		else if (author == "PlayerTwo" || author == "CoopTwoAI")
		{
			++_bulletHitByPlayerTwo;
		}
	}
}

void GameStatistics::EnemyHit(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		if (author.compare(0, 4, "Enemy"))
		{
			++_enemyHitByFriendlyFire;
		}
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "PlayerOne" || author == "CoopOneAI")
		{
			++_enemyHitByPlayerOne;
		}
		else if (author == "PlayerTwo" || author == "CoopTwoAI")
		{
			++_enemyHitByPlayerTwo;
		}
	}
}

void GameStatistics::PlayerOneHit(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		if (author.compare(0, 4, "Enemy"))
		{
			++_playerOneHitByEnemyTeam;
		}
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "PlayerOne" || author == "CoopOneAI" || author == "PlayerTwo" || author == "CoopTwoAI")
		{
			++_playerOneHitFriendlyFire;
		}
	}
}

void GameStatistics::PlayerTwoHit(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		if (author.compare(0, 4, "Enemy"))
		{
			++_playerTwoHitByEnemyTeam;
		}
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "PlayerOne" || author == "CoopOneAI" || author == "PlayerTwo" || author == "CoopOneTwo")
		{
			++_playerTwoHitFriendlyFire;
		}
	}
}

void GameStatistics::EnemyDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		if (author.compare(0, 4, "Enemy"))
		{
			++_enemyDiedByFriendlyFire;
		}
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "PlayerOne" || author == "CoopOneAI")
		{
			++_enemyDiedByPlayerOne;
		}
		else if (author == "PlayerTwo" || author == "CoopTwoAI")
		{
			++_enemyDiedByPlayerTwo;
		}
	}
}

void GameStatistics::PlayerOneDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		if (author.compare(0, 4, "Enemy"))
		{
			++_playerDiedByEnemyTeam;
		}
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "PlayerOne" || author == "CoopOneAI" || author == "PlayerTwo" || author == "CoopTwoAI")
		{
			++_playerOneDiedByFriendlyFire;
		}
	}
}

void GameStatistics::PlayerTwoDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		if (author.compare(0, 4, "Enemy"))
		{
			++_playerDiedByEnemyTeam;
		}
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "PlayerOne" || author == "CoopOneAI" || author == "PlayerTwo" || author == "CoopTwoAI")
		{
			++_playerTwoDiedByFriendlyFire;
		}
	}
}

void GameStatistics::BrickDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		if (author.compare(0, 4, "Enemy"))
		{
			++_brickDiedByEnemyTeam;
		}
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "PlayerOne" || author == "CoopOneAI")
		{
			++_brickDiedByPlayerOne;
		}
		else if (author == "PlayerTwo" || author == "CoopTwoAI")
		{
			++_brickDiedByPlayerTwo;
		}
	}
}

void GameStatistics::Reset()
{
	_enemyRespawnResource = 20;
	_playerOneRespawnResource = 3;
	_playerTwoRespawnResource = 3;

	_enemyOneNeedRespawn = false;
	_enemyTwoNeedRespawn = false;
	_enemyThreeNeedRespawn = false;
	_enemyFourNeedRespawn = false;

	_playerOneNeedRespawn = false;
	_playerTwoNeedRespawn = false;
	_coopOneAINeedRespawn = false;
	_coopTwoAINeedRespawn = false;

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

	_brickDiedByEnemyTeam = 0;
	_brickDiedByPlayerOne = 0;
	_brickDiedByPlayerTwo = 0;
}
