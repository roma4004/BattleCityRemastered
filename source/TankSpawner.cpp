#include "../headers/TankSpawner.h"
#include "../headers/EventSystem.h"
#include "../headers/input/InputProviderForPlayerOne.h"
#include "../headers/input/InputProviderForPlayerOneNet.h"
#include "../headers/input/InputProviderForPlayerTwo.h"
#include "../headers/input/InputProviderForPlayerTwoNet.h"
#include "../headers/pawns/CoopBot.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PlayerOne.h"
#include "../headers/pawns/PlayerTwo.h"
#include "../headers/utils/ColliderUtils.h"

#include <algorithm>
#include <memory>

TankSpawner::TankSpawner(std::shared_ptr<Window> window, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                         std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool)
	: _allObjects{allObjects},
	  _window{std::move(window)},
	  _events{std::move(events)},
	  _bulletPool{std::move(bulletPool)}
{
	Subscribe();
}

TankSpawner::~TankSpawner()
{
	Unsubscribe();
}

void TankSpawner::Subscribe()
{
	_events->AddListener("Reset", _name, [this]() { ResetSpawn(); });
	_events->AddListener("RespawnTanks", _name, [this]() { RespawnTanks(); });
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_currentMode = newGameMode;
	});

	SubscribeBonus();

	_events->AddListener("Enemy1_Died", _name, [this]() { _enemyOneNeedRespawn = true; });
	_events->AddListener("Enemy2_Died", _name, [this]() { _enemyTwoNeedRespawn = true; });
	_events->AddListener("Enemy3_Died", _name, [this]() { _enemyThreeNeedRespawn = true; });
	_events->AddListener("Enemy4_Died", _name, [this]() { _enemyFourNeedRespawn = true; });

	_events->AddListener("Player1_Died", _name, [this]() { _playerOneNeedRespawn = true; });
	_events->AddListener("Player2_Died", _name, [this]() { _playerTwoNeedRespawn = true; });
	_events->AddListener("CoopBot1_Died", _name, [this]() { _coopBotOneNeedRespawn = true; });
	_events->AddListener("CoopBot2_Died", _name, [this]() { _coopBotTwoNeedRespawn = true; });

	_events->AddListener("Player1_Spawn", _name, [this]()
	{
		_playerOneNeedRespawn = false;
		DecreasePlayerOneRespawnResource();
	});
	_events->AddListener("Player2_Spawn", _name, [this]()
	{
		_playerTwoNeedRespawn = false;
		DecreasePlayerTwoRespawnResource();
	});
	_events->AddListener("CoopBot1_Spawn", _name, [this]()
	{
		_coopBotOneNeedRespawn = false;
		DecreasePlayerOneRespawnResource();
	});
	_events->AddListener("CoopBot2_Spawn", _name, [this]()
	{
		_coopBotTwoNeedRespawn = false;
		DecreasePlayerTwoRespawnResource();
	});

	_events->AddListener("Enemy1_Spawn", _name, [this]()
	{
		_enemyOneNeedRespawn = false;
		DecreaseEnemyRespawnResource();
	});
	_events->AddListener("Enemy2_Spawn", _name, [this]()
	{
		_enemyTwoNeedRespawn = false;
		DecreaseEnemyRespawnResource();
	});
	_events->AddListener("Enemy3_Spawn", _name, [this]()
	{
		_enemyThreeNeedRespawn = false;
		DecreaseEnemyRespawnResource();
	});
	_events->AddListener("Enemy4_Spawn", _name, [this]()
	{
		_enemyFourNeedRespawn = false;
		DecreaseEnemyRespawnResource();
	});
}

void TankSpawner::SubscribeBonus()
{
	_events->AddListener<const std::string&, const std::string&>(
			"BonusTank", _name, [this](const std::string& author, const std::string& fraction)
			{
				if (fraction == "EnemyTeam")
				{
					IncreaseEnemyRespawnResource();
				}
				else if (fraction == "PlayerTeam")
				{
					if (author == "Player1" || author == "CoopBot1")
					{
						IncreasePlayerOneRespawnResource();
					}
					if (author == "Player2" || author == "CoopBot2")
					{
						IncreasePlayerTwoRespawnResource();
					}
				}
			});
}

void TankSpawner::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("RespawnTanks", _name);
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);

	UnsubscribeBonus();

	_events->RemoveListener("Enemy1_Died", _name);
	_events->RemoveListener("Enemy2_Died", _name);
	_events->RemoveListener("Enemy3_Died", _name);
	_events->RemoveListener("Enemy4_Died", _name);

	_events->RemoveListener("Player1_Died", _name);
	_events->RemoveListener("Player2_Died", _name);
	_events->RemoveListener("CoopBot1_Died", _name);
	_events->RemoveListener("CoopBot2_Died", _name);

	_events->RemoveListener("Player1_Spawn", _name);
	_events->RemoveListener("Player2_Spawn", _name);
	_events->RemoveListener("CoopBot1_Spawn", _name);
	_events->RemoveListener("CoopBot2_Spawn", _name);

	_events->RemoveListener("Enemy1_Spawn", _name);
	_events->RemoveListener("Enemy2_Spawn", _name);
	_events->RemoveListener("Enemy3_Spawn", _name);
	_events->RemoveListener("Enemy4_Spawn", _name);
}

void TankSpawner::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const std::string&>("BonusTank", _name);
}

void TankSpawner::SetEnemyNeedRespawn()
{
	_enemyOneNeedRespawn = true;
	_enemyTwoNeedRespawn = true;
	_enemyThreeNeedRespawn = true;
	_enemyFourNeedRespawn = true;
}

void TankSpawner::ResetRespawnStat()
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
	_coopBotOneNeedRespawn = false;
	_coopBotTwoNeedRespawn = false;
}

void TankSpawner::ResetSpawn()
{
	ResetRespawnStat();

	SetPlayerNeedRespawn();

	SetEnemyNeedRespawn();
}

void TankSpawner::SetPlayerNeedRespawn()
{
	if (_currentMode == Demo)
	{
		_coopBotOneNeedRespawn = true;
		_coopBotTwoNeedRespawn = true;

		return;
	}

	_playerOneNeedRespawn = true;

	if (_currentMode == CoopWithBot)
	{
		_coopBotTwoNeedRespawn = true;

		return;
	}

	if (_currentMode == TwoPlayers
	    || _currentMode == PlayAsHost
	    || _currentMode == PlayAsClient)
	{
		_playerTwoNeedRespawn = true;
	}
}

void TankSpawner::SpawnEnemy(const int index, const float gridOffset, const float speed, const int health,
                             const float size)
{
	std::vector<ObjRectangle> spawnPos{
			{.x = gridOffset * 16.f - size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 32.f - size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 16.f + size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 32.f + size * 2.f, .y = 0, .w = size, .h = size}
	};
	for (auto& rect: spawnPos)
	{
		const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
		{
			return ColliderUtils::IsCollide(rect, object->GetShape());
		});

		if (isFreeSpawnSpot)
		{
			constexpr int gray{0x808080};
			std::string name{"Enemy"};
			std::string fraction{"EnemyTeam"};

			_allObjects->emplace_back(
					std::make_shared<Enemy>(
							rect, gray, health, _window, DOWN, speed, _allObjects, _events, name, fraction, _bulletPool,
							_currentMode, index));

			return;
		}
	}
}

void TankSpawner::SpawnPlayer1(const float gridOffset, const float speed, const int health, const float size)
{
	const auto windowSizeY{static_cast<float>(_window->size.y)};
	const ObjRectangle rect{.x = gridOffset * 16.f, .y = windowSizeY - size, .w = size, .h = size};

	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetShape());
	});

	if (isFreeSpawnSpot)
	{
		constexpr int yellow{0xeaea00};
		const std::string name{"Player"};
		const std::string fraction{"PlayerTeam"};

		std::unique_ptr<IInputProvider> inputProvider;
		if (_currentMode == PlayAsClient)
		{
			inputProvider = std::make_unique<InputProviderForPlayerOneNet>(name, _events);
		}
		else
		{
			inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		}

		_allObjects->emplace_back(
				std::make_shared<PlayerOne>(
						rect, yellow, health, _window, UP, speed, _allObjects, _events, name, fraction,
						std::move(inputProvider), _bulletPool, _currentMode, 1));
	}
}

void TankSpawner::SpawnPlayer2(const float gridOffset, const float speed, const int health, const float size)
{
	const auto windowSizeY{static_cast<float>(_window->size.y)};
	const ObjRectangle rect{.x = gridOffset * 32.f, .y = windowSizeY - size, .w = size, .h = size};
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetShape());
	});

	if (isFreeSpawnSpot)
	{
		constexpr int green{0x408000};
		const std::string name{"Player"};
		const std::string fraction{"PlayerTeam"};

		std::unique_ptr<IInputProvider> inputProvider;
		if (_currentMode == PlayAsClient || _currentMode == PlayAsHost)
		{
			inputProvider = std::make_unique<InputProviderForPlayerTwoNet>(name, _events);
		}
		else
		{
			inputProvider = std::make_unique<InputProviderForPlayerTwo>(name, _events);
		}

		_allObjects->emplace_back(
				std::make_shared<PlayerTwo>(
						rect, green, health, _window, UP, speed, _allObjects, _events, name, fraction,
						std::move(inputProvider), _bulletPool, _currentMode, 2));
	}
}

void TankSpawner::SpawnCoopBot1(const float gridOffset, const float speed, const int health, const float size)
{
	const auto windowSizeY{static_cast<float>(_window->size.y)};
	const ObjRectangle rect{.x = gridOffset * 16.f, .y = windowSizeY - size, .w = size, .h = size};
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetShape());
	});

	if (isFreeSpawnSpot)
	{
		constexpr int yellow{0xeaea00};
		std::string name{"CoopBot"};
		std::string fraction{"PlayerTeam"};

		_allObjects->emplace_back(
				std::make_shared<CoopBot>(
						rect, yellow, health, _window, UP, speed, _allObjects, _events, name, fraction, _bulletPool,
						_currentMode, 1));
	}
}

void TankSpawner::SpawnCoopBot2(const float gridOffset, const float speed, const int health, const float size)
{
	const auto windowSizeY{static_cast<float>(_window->size.y)};
	const ObjRectangle rect{.x = gridOffset * 32.f, .y = windowSizeY - size, .w = size, .h = size};
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetShape());
	});

	if (isFreeSpawnSpot)
	{
		constexpr int green{0x408000};
		std::string name{"CoopBot"};
		std::string fraction{"PlayerTeam"};

		_allObjects->emplace_back(std::make_shared<CoopBot>(
				rect, green, health, _window, UP, speed, _allObjects, _events, name, fraction, _bulletPool,
				_currentMode, 2));
	}
}

void TankSpawner::RespawnEnemyTanks(const int index)
{
	if (GetEnemyRespawnResource() > 0)
	{
		const float gridOffset{static_cast<float>(_window->size.y) / 50.f};
		const float size{gridOffset * 3};
		constexpr float speed{142};
		constexpr int health{100};

		SpawnEnemy(index, gridOffset, speed, health, size);
	}
	else
	{
		_enemyOneNeedRespawn = false;
		_events->EmitEvent<const std::string>("ServerSend_TankDied", "Enemy" + std::to_string(index));
	}
}

void TankSpawner::RespawnPlayerTanks(const int index)
{
	const float gridOffset{static_cast<float>(_window->size.y) / 50.f};
	const float size{gridOffset * 3};
	constexpr float speed{142};
	constexpr int health{100};

	if (index == 1)
	{
		if (GetPlayerOneRespawnResource() > 0)
		{
			SpawnPlayer1(gridOffset, speed, health, size);
		}
		else
		{
			_playerOneNeedRespawn = false;
			_events->EmitEvent<const std::string>("ServerSend_TankDied", "Player" + std::to_string(index));
		}
	}
	else if (index == 2)
	{
		if (GetPlayerTwoRespawnResource() > 0)
		{
			SpawnPlayer2(gridOffset, speed, health, size);
		}
		else
		{
			_playerTwoNeedRespawn = false;
			_events->EmitEvent<const std::string>("ServerSend_TankDied", "Player" + std::to_string(index));
		}
	}
}

void TankSpawner::RespawnCoopTanks(const int index)
{
	const float gridOffset{static_cast<float>(_window->size.y) / 50.f};
	const float size{gridOffset * 3};
	constexpr float speed{142};
	constexpr int health{100};

	if (index == 1)
	{
		if (GetPlayerOneRespawnResource() > 0)
		{
			SpawnCoopBot1(gridOffset, speed, health, size);
		}
		else
		{
			_coopBotOneNeedRespawn = false;
		}
	}
	else if (index == 2)
	{
		if (GetPlayerTwoRespawnResource() > 0)
		{
			SpawnCoopBot2(gridOffset, speed, health, size);
		}
		else
		{
			_coopBotTwoNeedRespawn = false;
		}
	}
}

void TankSpawner::RespawnTanks()
{
	if (IsEnemyOneNeedRespawn()) { RespawnEnemyTanks(1); }

	if (IsEnemyTwoNeedRespawn()) { RespawnEnemyTanks(2); }

	if (IsEnemyThreeNeedRespawn()) { RespawnEnemyTanks(3); }

	if (IsEnemyFourNeedRespawn()) { RespawnEnemyTanks(4); }

	if (IsPlayerOneNeedRespawn()) { RespawnPlayerTanks(1); }

	if (IsPlayerTwoNeedRespawn()) { RespawnPlayerTanks(2); }

	if (IsCoopBotOneNeedRespawn()) { RespawnCoopTanks(1); }

	if (IsCoopBotTwoNeedRespawn()) { RespawnCoopTanks(2); }
}

void TankSpawner::IncreaseEnemyRespawnResource()
{
	++_enemyRespawnResource;
	_events->EmitEvent<const int>("EnemyRespawnResourceChangedTo", _enemyRespawnResource);
}

void TankSpawner::IncreasePlayerOneRespawnResource()
{
	++_playerOneRespawnResource;
	_events->EmitEvent<const int>("Player1RespawnResourceChangedTo", _playerOneRespawnResource);
}

void TankSpawner::IncreasePlayerTwoRespawnResource()
{
	++_playerTwoRespawnResource;
	_events->EmitEvent<const int>("Player2RespawnResourceChangedTo", _playerTwoRespawnResource);
}

void TankSpawner::DecreaseEnemyRespawnResource()
{
	--_enemyRespawnResource;
	_events->EmitEvent<const int>("EnemyRespawnResourceChangedTo", _enemyRespawnResource);
}

void TankSpawner::DecreasePlayerOneRespawnResource()
{
	--_playerOneRespawnResource;
	_events->EmitEvent<const int>("Player1RespawnResourceChangedTo", _playerOneRespawnResource);
}

void TankSpawner::DecreasePlayerTwoRespawnResource()
{
	--_playerTwoRespawnResource;
	_events->EmitEvent<const int>("Player2RespawnResourceChangedTo", _playerTwoRespawnResource);
}
