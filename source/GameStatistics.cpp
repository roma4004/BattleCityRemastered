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
		++_bulletHitByEnemy;
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
		++_enemyHitByFriendlyFire;
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
		++_playerOneHitByEnemyTeam;
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
		++_playerTwoHitByEnemyTeam;
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
		++_enemyDiedByFriendlyFire;
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
		++_playerDiedByEnemyTeam;
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
		++_playerDiedByEnemyTeam;
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
		++_brickDiedByEnemyTeam;
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
