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
	_events->AddListener("Reset", _name, [this]() { Reset(); });

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
			"CoopBot1Hit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerOneHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"CoopBot1Died",
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
			"CoopBot2Hit",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerTwoHit(author, fraction); });
	_events->AddListener<const std::string&, const std::string&>(
			"CoopBot2Died",
			_name,
			[this](const std::string& author, const std::string& fraction) { PlayerTwoDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"BrickWallDied",
			_name,
			[this](const std::string& author, const std::string& fraction) { BrickWallDied(author, fraction); });
}

void GameStatistics::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);

	_events->RemoveListener<const std::string&, const std::string&>("BulletHit", _name);

	_events->RemoveListener<const std::string&, const std::string&>("EnemyHit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("EnemyDied", _name);

	_events->RemoveListener<const std::string&, const std::string&>("Player1Hit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("Player1Died", _name);

	_events->RemoveListener<const std::string&, const std::string&>("CoopBot1Hit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("CoopBot1Died", _name);

	_events->RemoveListener<const std::string&, const std::string&>("Player2Hit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("Player2Died", _name);

	_events->RemoveListener<const std::string&, const std::string&>("CoopBot2Hit", _name);
	_events->RemoveListener<const std::string&, const std::string&>("CoopBot2Died", _name);

	_events->RemoveListener<const std::string&, const std::string&>("BrickWallDied", _name);
}

void GameStatistics::BulletHit(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_bulletHitByEnemy;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopBot1")
		{
			++_bulletHitByPlayerOne;
		}
		else if (author == "Player2" || author == "CoopBot2")
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
		if (author == "Player1" || author == "CoopBot1")
		{
			++_enemyHitByPlayerOne;
		}
		else if (author == "Player2" || author == "CoopBot2")
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
		if (author == "Player1" || author == "CoopBot1" || author == "Player2" || author == "CoopBot2")
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
		if (author == "Player1" || author == "CoopBot1" || author == "Player2" || author == "CoopBot2")
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
		if (author == "Player1" || author == "CoopBot1")
		{
			++_enemyDiedByPlayerOne;
		}
		else if (author == "Player2" || author == "CoopBot2")
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
		if (author == "Player1" || author == "CoopBot1" || author == "Player2" || author == "CoopBot2")
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
		if (author == "Player1" || author == "CoopBot1" || author == "Player2" || author == "CoopBot2")
		{
			++_playerTwoDiedByFriendlyFire;
		}
	}
}

void GameStatistics::BrickWallDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_brickWallDiedByEnemyTeam;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopBot1")
		{
			++_brickWallDiedByPlayerOne;
		}
		else if (author == "Player2" || author == "CoopBot2")
		{
			++_brickWallDiedByPlayerTwo;
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

	_brickWallDiedByEnemyTeam = 0;
	_brickWallDiedByPlayerOne = 0;
	_brickWallDiedByPlayerTwo = 0;
}
