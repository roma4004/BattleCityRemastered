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

GameStatistics::~GameStatistics()
{
	Unsubscribe();
}

void GameStatistics::Subscribe()
{
	_events->AddListener(_name, [this](const GameResetEvent&) { Reset(); });
	_events->AddListener(_name, [this](const GameModeChangedToEvent& event)
	{
		this->OnGameModeChangedTo(event.mode);
	});

	SubscribeHost();
}

void GameStatistics::SubscribeHost()
{
	_events->AddListener(_name, [this](const StatisticsBulletHitEvent& event)
	{
		this->OnBulletHit(event);
	});

	_events->AddListener(
			_name,
			[this](const StatisticsTankHitEvent& event)
			{
				this->OnTankHit(event);
			});

	_events->AddListener(
			_name,
			[this](const StatisticsTankDiedEvent& event)
			{
				this->OnTankDied(event);
			});

	_events->AddListener(
			_name,
			[this](const BrickWallDiedEvent& event)
			{
				OnBrickWallDied(StatisticsAttributionEvent{.author = event.author, .fraction = event.fraction});
			});

	_events->AddListener(
			_name,
			[this](const SteelWallDiedEvent& event)
			{
				OnSteelWallDied(StatisticsAttributionEvent{.author = event.author, .fraction = event.fraction});
			});

	_events->AddListener(
			_name,
			[this](const StatisticsBonusPickupEvent& event)
			{
				OnBonusPickup(event);
			});

	_events->AddListener(
			_name,
			[this](const StatisticsBonusDestroyedEvent& event)
			{
				OnBonusDestroyed(event);
			});
}

void GameStatistics::SubscribeAsClient()
{
	_events->AddListener(_name, [this](const ClientReceivedBulletHitEvent& event)
	{
		OnBulletHit(StatisticsBulletHitEvent{.author = event.author, .fraction = event.fraction});
	});

	_events->AddListener(_name, [this](const ClientReceivedEnemyHitEvent& event)
	{
		OnEnemyHit(event.author, event.fraction);
	});

	_events->AddListener(_name, [this](const ClientReceivedPlayerOneHitEvent& event)
	{
		OnPlayerOneHit(event.author, event.fraction);
	});

	_events->AddListener(_name, [this](const ClientReceivedPlayerTwoHitEvent& event)
	{
		OnPlayerTwoHit(event.author, event.fraction);
	});

	_events->AddListener(_name, [this](const ClientReceivedEnemyDiedEvent& event)
	{
		OnEnemyDied(event.author, event.fraction);
	});

	_events->AddListener(_name, [this](const ClientReceivedPlayerOneDiedEvent& event)
	{
		OnPlayerOneDied(event.author, event.fraction);
	});

	_events->AddListener(_name, [this](const ClientReceivedPlayerTwoDiedEvent& event)
	{
		OnPlayerTwoDied(event.author, event.fraction);
	});

	_events->AddListener(_name, [this](const ClientReceivedBrickWallDiedEvent& event)
	{
		OnBrickWallDied(StatisticsAttributionEvent{.author = event.author, .fraction = event.fraction});
	});

	_events->AddListener(_name, [this](const ClientReceivedSteelWallDiedEvent& event)
	{
		OnSteelWallDied(StatisticsAttributionEvent{.author = event.author, .fraction = event.fraction});
	});

	_events->AddListener(_name, [this](const ClientReceivedBonusPickupEvent& event)
	{
		OnBonusPickup(StatisticsBonusPickupEvent{.author = event.author, .fraction = event.fraction});
	});

	_events->AddListener(_name, [this](const ClientReceivedBonusDestroyedEvent& event)
	{
		OnBonusDestroyed(StatisticsBonusDestroyedEvent{.author = event.author, .fraction = event.fraction});
	});
}

void GameStatistics::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void GameStatistics::UnsubscribeAsHost() const
{
	_events->RemoveListener<StatisticsBulletHitEvent>(_name);
	_events->RemoveListener<StatisticsTankHitEvent>(_name);
	_events->RemoveListener<StatisticsTankDiedEvent>(_name);

	_events->RemoveListener<BrickWallDiedEvent>(_name);
	_events->RemoveListener<SteelWallDiedEvent>(_name);
	_events->RemoveListener<StatisticsBonusPickupEvent>(_name);
}

void GameStatistics::UnsubscribeAsClient() const
{
	_events->RemoveListener<ClientReceivedBulletHitEvent>(_name);
	_events->RemoveListener<ClientReceivedEnemyHitEvent>(_name);
	_events->RemoveListener<ClientReceivedPlayerOneHitEvent>(_name);
	_events->RemoveListener<ClientReceivedPlayerTwoHitEvent>(_name);
	_events->RemoveListener<ClientReceivedEnemyDiedEvent>(_name);
	_events->RemoveListener<ClientReceivedPlayerOneDiedEvent>(_name);
	_events->RemoveListener<ClientReceivedPlayerTwoDiedEvent>(_name);
	_events->RemoveListener<ClientReceivedBrickWallDiedEvent>(_name);
	_events->RemoveListener<ClientReceivedSteelWallDiedEvent>(_name);
	_events->RemoveListener<ClientReceivedBonusPickupEvent>(_name);
	_events->RemoveListener<ClientReceivedBonusDestroyedEvent>(_name);
}

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
		_events->EmitEvent(ServerSendBulletHitEvent{.author = event.author, .fraction = event.fraction});
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
		_events->EmitEvent(ServerSendEnemyHitEvent{.author = author, .fraction = fraction});
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
		_events->EmitEvent(ServerSendPlayerOneHitEvent{.author = author, .fraction = fraction});
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
		_events->EmitEvent(ServerSendPlayerTwoHitEvent{.author = author, .fraction = fraction});
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
		_events->EmitEvent(ServerSendEnemyDiedEvent{.author = author, .fraction = fraction});
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
		_events->EmitEvent(ServerSendPlayerOneDiedEvent{.author = author, .fraction = fraction});
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
		_events->EmitEvent(ServerSendPlayerTwoDiedEvent{.author = author, .fraction = fraction});
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
		_events->EmitEvent(ServerSendBrickWallDiedEvent{.author = event.author, .fraction = event.fraction});
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
		_events->EmitEvent(ServerSendSteelWallDiedEvent{.author = event.author, .fraction = event.fraction});
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
		_events->EmitEvent(ServerSendBonusPickupEvent{.author = event.author, .fraction = event.fraction});
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
		_events->EmitEvent(ServerSendBonusDestroyedEvent{.author = event.author, .fraction = event.fraction});
	}
}

void GameStatistics::Reset() { _data = {}; }
