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

	_events->AddListener("Enemy1_Died", _name, [this]() { enemyOneNeedRespawn = true; });
	_events->AddListener("Enemy2_Died", _name, [this]() { enemyTwoNeedRespawn = true; });
	_events->AddListener("Enemy3_Died", _name, [this]() { enemyThreeNeedRespawn = true; });
	_events->AddListener("Enemy4_Died", _name, [this]() { enemyFourNeedRespawn = true; });

	_events->AddListener("PlayerOne_Died", _name, [this]() { playerOneNeedRespawn = true; });
	_events->AddListener("PlayerTwo_Died", _name, [this]() { playerTwoNeedRespawn = true; });
	_events->AddListener("CoopOneAI_Died", _name, [this]() { coopOneAINeedRespawn = true; });
	_events->AddListener("CoopTwoAI_Died", _name, [this]() { coopTwoAINeedRespawn = true; });

	_events->AddListener("PlayerOne_Spawn", _name, [this]()
	{
		playerOneNeedRespawn = false;
		--playerOneRespawnResource;
	});
	_events->AddListener("PlayerTwo_Spawn", _name, [this]()
	{
		playerTwoNeedRespawn = false;
		--playerTwoRespawnResource;
	});
	_events->AddListener("CoopOneAI_Spawn", _name, [this]()
	{
		coopOneAINeedRespawn = false;
		--playerOneRespawnResource;
	});
	_events->AddListener("CoopTwoAI_Spawn", _name, [this]()
	{
		coopTwoAINeedRespawn = false;
		--playerTwoRespawnResource;
	});

	_events->AddListener("Enemy1_Spawn", _name, [this]()
	{
		enemyOneNeedRespawn = false;
		--enemyRespawnResource;
	});
	_events->AddListener("Enemy2_Spawn", _name, [this]()
	{
		enemyTwoNeedRespawn = false;
		--enemyRespawnResource;
	});
	_events->AddListener("Enemy3_Spawn", _name, [this]()
	{
		enemyThreeNeedRespawn = false;
		--enemyRespawnResource;
	});
	_events->AddListener("Enemy4_Spawn", _name, [this]()
	{
		enemyFourNeedRespawn = false;
		--enemyRespawnResource;
	});

	//TODO: replace <std::string> with <Enum::statisticsType>
	_events->AddListener<std::string>("BulletHit", _name, [this](const std::string& by) { BulletHit(by); });

	_events->AddListener<std::string>("EnemyHit", _name, [this](const std::string& by) { EnemyHit(by); });
	_events->AddListener<std::string>("EnemyDied", _name, [this](const std::string& by) { EnemyDied(by); });

	_events->AddListener<std::string>("PlayerOneHit", _name, [this](const std::string& by) { PlayerOneHit(by); });
	_events->AddListener<std::string>("PlayerOneDied", _name, [this](const std::string& by) { PlayerDied(by); });

	_events->AddListener<std::string>("CoopOneAIHit", _name, [this](const std::string& by) { PlayerOneHit(by); });
	_events->AddListener<std::string>("CoopOneAIDied", _name, [this](const std::string& by) { PlayerDied(by); });

	_events->AddListener<std::string>("PlayerTwoHit", _name, [this](const std::string& by) { PlayerTwoHit(by); });
	_events->AddListener<std::string>("PlayerTwoDied", _name, [this](const std::string& by) { PlayerDied(by); });

	_events->AddListener<std::string>("CoopTwoAIHit", _name, [this](const std::string& by) { PlayerTwoHit(by); });
	_events->AddListener<std::string>("CoopTwoAIDied", _name, [this](const std::string& by) { PlayerDied(by); });

	_events->AddListener<std::string>("BrickDied", _name, [this](const std::string& by) { BrickDied(by); });
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

	_events->RemoveListener("P1_Died", _name);
	_events->RemoveListener("P2_Died", _name);
	_events->RemoveListener("CoopOneAI_Died", _name);
	_events->RemoveListener("CoopTwoAI_Died", _name);

	_events->RemoveListener("P1_Spawn", _name);
	_events->RemoveListener("P2_Spawn", _name);
	_events->RemoveListener("CoopOneAI_Spawn", _name);
	_events->RemoveListener("CoopTwoAI_Spawn", _name);

	_events->RemoveListener("Enemy1_Spawn", _name);
	_events->RemoveListener("Enemy2_Spawn", _name);
	_events->RemoveListener("Enemy3_Spawn", _name);
	_events->RemoveListener("Enemy4_Spawn", _name);

	_events->RemoveListener<std::string>("BulletHit", _name);

	_events->RemoveListener<std::string>("EnemyHit", _name);
	_events->RemoveListener<std::string>("EnemyDied", _name);

	_events->RemoveListener<std::string>("PlayerOneHit", _name);
	_events->RemoveListener<std::string>("PlayerOneDied", _name);

	_events->RemoveListener<std::string>("CoopOneAIHit", _name);
	_events->RemoveListener<std::string>("CoopOneAIDied", _name);

	_events->RemoveListener<std::string>("PlayerTwoHit", _name);
	_events->RemoveListener<std::string>("PlayerTwoDied", _name);

	_events->RemoveListener<std::string>("CoopTwoAIHit", _name);
	_events->RemoveListener<std::string>("CoopTwoAIDied", _name);

	_events->RemoveListener<std::string>("BrickDied", _name);
}

void GameStatistics::BulletHit(const std::string& authorTeam)
{
	if (authorTeam == "Enemy1EnemyTeam" || authorTeam == "Enemy2EnemyTeam"
	    || authorTeam == "Enemy3EnemyTeam" || authorTeam == "Enemy4EnemyTeam")
	{
		++bulletHitByEnemy;
	}
	if (authorTeam == "PlayerOnePlayerTeam" || authorTeam == "CoopOneAIPlayerTeam")
	{
		++bulletHitByPlayerOne;
	}
	else if (authorTeam == "PlayerTwoPlayerTeam" || authorTeam == "CoopTwoAIPlayerTeam")
	{
		++bulletHitByPlayerTwo;
	}
}

void GameStatistics::EnemyHit(const std::string& authorTeam)
{
	if (authorTeam == "Enemy1EnemyTeam" || authorTeam == "Enemy2EnemyTeam"
	    || authorTeam == "Enemy3EnemyTeam" || authorTeam == "Enemy4EnemyTeam")
	{
		++enemyHitByFriendlyFire;
	}
	if (authorTeam == "PlayerOnePlayerTeam" || authorTeam == "CoopOneAIPlayerTeam")
	{
		++enemyHitByPlayerOne;
	}
	else if (authorTeam == "PlayerTwoPlayerTeam" || authorTeam == "CoopTwoAIPlayerTeam")
	{
		++enemyHitByPlayerTwo;
	}
}

void GameStatistics::PlayerOneHit(const std::string& authorTeam)
{
	if (authorTeam == "Enemy1EnemyTeam" || authorTeam == "Enemy2EnemyTeam"
	    || authorTeam == "Enemy3EnemyTeam" || authorTeam == "Enemy4EnemyTeam")
	{
		++playerOneHitByEnemyTeam;
	}
	if (authorTeam == "PlayerTwoPlayerTeam" || authorTeam == "CoopTwoAIPlayerTeam")
	{
		++playerOneHitFriendlyFire;
	}
}

void GameStatistics::PlayerTwoHit(const std::string& authorTeam)
{
	if (authorTeam == "Enemy1EnemyTeam" || authorTeam == "Enemy2EnemyTeam"
	    || authorTeam == "Enemy3EnemyTeam" || authorTeam == "Enemy4EnemyTeam")
	{
		++playerTwoHitByEnemyTeam;
	}
	else if (authorTeam == "PlayerOnePlayerTeam" || authorTeam == "CoopOneAIPlayerTeam")
	{
		++playerTwoHitFriendlyFire;
	}
}

void GameStatistics::EnemyDied(const std::string& authorTeam)
{
	if (authorTeam == "Enemy1EnemyTeam" || authorTeam == "Enemy2EnemyTeam"
	    || authorTeam == "Enemy3EnemyTeam" || authorTeam == "Enemy4EnemyTeam")
	{
		++enemyDiedByFriendlyFire;
	}
	else if (authorTeam == "PlayerOnePlayerTeam" || authorTeam == "CoopOneAIPlayerTeam")
	{
		++enemyDiedByPlayerOne;
	}
	else if (authorTeam == "PlayerTwoPlayerTeam" || authorTeam == "CoopTwoAIPlayerTeam")
	{
		++enemyDiedByPlayerTwo;
	}
}

void GameStatistics::PlayerDied(const std::string& authorTeam)
{
	if (authorTeam == "Enemy1EnemyTeam" || authorTeam == "Enemy2EnemyTeam"
	    || authorTeam == "Enemy3EnemyTeam" || authorTeam == "Enemy4EnemyTeam")
	{
		++playerDiedByEnemyTeam;
	}
	else if (authorTeam == "PlayerOnePlayerTeam" || authorTeam == "CoopOneAIPlayerTeam")
	{
		++playerTwoDiedByFriendlyFire;
	}
	else if (authorTeam == "PlayerTwoPlayerTeam" || authorTeam == "CoopTwoAIPlayerTeam")
	{
		++playerOneDiedByFriendlyFire;
	}
}

void GameStatistics::BrickDied(const std::string& authorTeam)
{
	if (authorTeam == "Enemy1EnemyTeam" || authorTeam == "Enemy2EnemyTeam"
	    || authorTeam == "Enemy3EnemyTeam" || authorTeam == "Enemy4EnemyTeam")
	{
		++brickDiedByEnemyTeam;
	}
	else if (authorTeam == "PlayerOnePlayerTeam" || authorTeam == "CoopOneAIPlayerTeam")
	{
		++brickDiedByPlayerOne;
	}
	else if (authorTeam == "PlayerTwoPlayerTeam" || authorTeam == "CoopTwoAIPlayerTeam")
	{
		++brickDiedByPlayerTwo;
	}
}

void GameStatistics::Reset()
{
	enemyRespawnResource = 20;
	playerOneRespawnResource = 3;
	playerTwoRespawnResource = 3;

	enemyOneNeedRespawn = false;
	enemyTwoNeedRespawn = false;
	enemyThreeNeedRespawn = false;
	enemyFourNeedRespawn = false;

	playerOneNeedRespawn = false;
	playerTwoNeedRespawn = false;
	coopOneAINeedRespawn = false;
	coopTwoAINeedRespawn = false;

	bulletHitByEnemy = 0;
	bulletHitByPlayerOne = 0;
	bulletHitByPlayerTwo = 0;

	enemyHitByFriendlyFire = 0;
	enemyHitByPlayerOne = 0;
	enemyHitByPlayerTwo = 0;

	playerOneHitFriendlyFire = 0;
	playerOneHitByEnemyTeam = 0;

	playerTwoHitFriendlyFire = 0;
	playerTwoHitByEnemyTeam = 0;

	enemyDiedByFriendlyFire = 0;
	enemyDiedByPlayerOne = 0;
	enemyDiedByPlayerTwo = 0;

	playerOneDiedByFriendlyFire = 0;
	playerTwoDiedByFriendlyFire = 0;
	playerDiedByEnemyTeam = 0;

	brickDiedByEnemyTeam = 0;
	brickDiedByPlayerOne = 0;
	brickDiedByPlayerTwo = 0;
}
