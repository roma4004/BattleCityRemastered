#include "../headers/GameStatistics.h"
#include "../headers/EventSystem.h"

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
			"Player1Hit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerOneHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"Player1Died",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerOneDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"CoopAI1Hit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerOneHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"CoopAI1Died",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerOneDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"Player2Hit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerTwoHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"Player2Died",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerTwoDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"CoopAI2Hit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerTwoHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"CoopAI2Died",
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

	_events->RemoveListener<const std::string&, const std::string&>("BulletHit", _name);

	_events->RemoveListener<const std::string&, const std::string&>("EnemyHit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("EnemyDied", _name);

	_events->RemoveListener<const std::string&, const std::string&>("Player1Hit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("Player1Died", _name);

	_events->RemoveListener<const std::string&, const std::string&>("CoopAI1Hit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("CoopAI1Died", _name);

	_events->RemoveListener<const std::string&, const std::string&>("Player2Hit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("Player2Died", _name);

	_events->RemoveListener<const std::string&, const std::string&>("CoopAI2Hit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("CoopAI2Died", _name);

	_events->RemoveListener<const std::string&, const std::string&>("BrickDied", _name);
}

void GameStatistics::BulletHit(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_bulletHitByEnemy;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopAI1")
		{
			++_bulletHitByPlayerOne;
		}
		else if (author == "Player2" || author == "CoopAI2")
		{
			++_bulletHitByPlayerTwo;
		}
	}
}

void GameStatistics::EnemyHit(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_enemyHitByFriendlyFire;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopAI1")
		{
			++_enemyHitByPlayerOne;
		}
		else if (author == "Player2" || author == "CoopAI2")
		{
			++_enemyHitByPlayerTwo;
		}
	}
}

void GameStatistics::PlayerOneHit(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_playerOneHitByEnemyTeam;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopAI1" || author == "Player2" || author == "CoopAI2")
		{
			++_playerOneHitFriendlyFire;
		}
	}
}

void GameStatistics::PlayerTwoHit(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_playerTwoHitByEnemyTeam;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopAI1" || author == "Player2" || author == "CoopAI2")
		{
			++_playerTwoHitFriendlyFire;
		}
	}
}

void GameStatistics::EnemyDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_enemyDiedByFriendlyFire;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopAI1")
		{
			++_enemyDiedByPlayerOne;
		}
		else if (author == "Player2" || author == "CoopAI2")
		{
			++_enemyDiedByPlayerTwo;
		}
	}
}

void GameStatistics::PlayerOneDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_playerDiedByEnemyTeam;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopAI1" || author == "Player2" || author == "CoopAI2")
		{
			++_playerOneDiedByFriendlyFire;
		}
	}
}

void GameStatistics::PlayerTwoDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_playerDiedByEnemyTeam;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopAI1" || author == "Player2" || author == "CoopAI2")
		{
			++_playerTwoDiedByFriendlyFire;
		}
	}
}

void GameStatistics::BrickDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_brickDiedByEnemyTeam;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopAI1")
		{
			++_brickDiedByPlayerOne;
		}
		else if (author == "Player2" || author == "CoopAI2")
		{
			++_brickDiedByPlayerTwo;
		}
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

	_brickDiedByEnemyTeam = 0;
	_brickDiedByPlayerOne = 0;
	_brickDiedByPlayerTwo = 0;
}
