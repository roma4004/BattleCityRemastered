#include "../../headers/components/GameStatistics.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"

GameStatistics::GameStatistics(std::shared_ptr<EventSystem> events)
	: _name{"Statistics"}, _events{std::move(events)}
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
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		if (_gameMode == PlayAsClient)
		{
			UnsubscribeAsHost();
			SubscribeAsClient();
		}
		else
		{
			UnsubscribeAsClient();
			SubscribeHost();
		}
	});

	SubscribeHost();
}

void GameStatistics::SubscribeHost()
{
	//TODO: replace <std::string> with <Enum::statisticsType>
	_events->AddListener<const std::string&, const std::string&>(
			"Statistics_BulletHit", _name,
			[this](const std::string& author, const std::string& fraction) { BulletHit(author, fraction); });

	_events->AddListener<const std::string&, const std::string&, const std::string&>(
			"Statistics_TankHit", _name,
			[this](const std::string& whoHit, const std::string& author, const std::string& fraction)
			{
				if (whoHit == "Enemy1" || whoHit == "Enemy2" || whoHit == "Enemy3" || whoHit == "Enemy4")
				{
					EnemyHit(author, fraction);
				}
				else if (whoHit == "Player1" || whoHit == "CoopBot1")
				{
					PlayerOneHit(author, fraction);
				}
				else if (whoHit == "Player2" || whoHit == "CoopBot2")
				{
					PlayerTwoHit(author, fraction);
				}
			});

	_events->AddListener<const std::string&, const std::string&, const std::string&>(
			"Statistics_TankDied", _name,
			[this](const std::string& whoDied, const std::string& author, const std::string& fraction)
			{
				if (whoDied == "Enemy1" || whoDied == "Enemy2" || whoDied == "Enemy3" || whoDied == "Enemy4")
				{
					EnemyDied(author, fraction);
				}
				else if (whoDied == "Player1" || whoDied == "CoopBot1")
				{
					PlayerOneDied(author, fraction);
				}
				else if (whoDied == "Player2" || whoDied == "CoopBot2")
				{
					PlayerTwoDied(author, fraction);
				}
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BrickWallDied", _name,
			[this](const std::string& author, const std::string& fraction) { BrickWallDied(author, fraction); });

	_events->AddListener<const std::string&, const std::string&>(
			"SteelWallDied", _name,
			[this](const std::string& author, const std::string& fraction) { SteelWallDied(author, fraction); });
}

void GameStatistics::SubscribeAsClient()
{
	_events->AddListener<const std::string&, const std::string&, const std::string&>("ClientReceived_Statistics", _name,
		[this](const std::string& type, const std::string& author, const std::string& fraction)
		{
			ClientGateway(type, author, fraction);
		});
}

void GameStatistics::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);

	_gameMode == PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();
}

void GameStatistics::UnsubscribeAsHost() const
{
	_events->RemoveListener<const std::string&, const std::string&>("Statistics_BulletHit", _name);
	_events->RemoveListener<const std::string&, const std::string&, const std::string&>("Statistics_TankHit", _name);
	_events->RemoveListener<const std::string&, const std::string&, const std::string&>("Statistics_TankDied", _name);

	_events->RemoveListener<const std::string&, const std::string&>("BrickWallDied", _name);
	_events->RemoveListener<const std::string&, const std::string&>("SteelWallDied", _name);
}

void GameStatistics::UnsubscribeAsClient() const
{
	_events->RemoveListener<const std::string&, const std::string&, const std::string&>(
			"ClientReceived_Statistics", _name);
}

void GameStatistics::ClientGateway(const std::string& type, const std::string& author, const std::string& fraction)
{
	if (type == "BulletHit")
	{
		BulletHit(author, fraction);
	}
	else if (type == "EnemyHit")
	{
		EnemyHit(author, fraction);
	}
	else if (type == "PlayerOneHit")
	{
		PlayerOneHit(author, fraction);
	}
	else if (type == "PlayerTwoHit")
	{
		PlayerTwoHit(author, fraction);
	}
	else if (type == "EnemyDied")
	{
		EnemyDied(author, fraction);
	}
	else if (type == "PlayerOneDied")
	{
		PlayerOneDied(author, fraction);
	}
	else if (type == "PlayerTwoDied")
	{
		PlayerTwoDied(author, fraction);
	}
	else if (type == "BrickWallDied")
	{
		BrickWallDied(author, fraction);
	}
	else if (type == "SteelWallDied")
	{
		SteelWallDied(author, fraction);
	}
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

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ServerSend_Statistics", "BulletHit", author, fraction);
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

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ServerSend_Statistics", "EnemyHit", author, fraction);
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

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ServerSend_Statistics", "PlayerOneHit", author, fraction);
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

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ServerSend_Statistics", "PlayerTwoHit", author, fraction);
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

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ServerSend_Statistics", "EnemyDied", author, fraction);
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

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ServerSend_Statistics", "PlayerOneDied", author, fraction);
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

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ServerSend_Statistics", "PlayerTwoDied", author, fraction);
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

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ServerSend_Statistics", "BrickWallDied", author, fraction);
	}
}

void GameStatistics::SteelWallDied(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		++_steelWallDiedByEnemyTeam;
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopBot1")
		{
			++_steelWallDiedByPlayerOne;
		}
		else if (author == "Player2" || author == "CoopBot2")
		{
			++_steelWallDiedByPlayerTwo;
		}
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ServerSend_Statistics", "SteelWallDied", author, fraction);
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
