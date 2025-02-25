#include "../headers/TankSpawner.h"
#include "../headers/EventSystem.h"
#include "../headers/input/InputProviderForPlayerOne.h"
#include "../headers/input/InputProviderForPlayerOneNet.h"
#include "../headers/input/InputProviderForPlayerTwo.h"
#include "../headers/input/InputProviderForPlayerTwoNet.h"
#include "../headers/pawns/CoopAI.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PlayerOne.h"
#include "../headers/pawns/PlayerTwo.h"
#include "../headers/utils/ColliderUtils.h"

#include <algorithm>
#include <memory>

TankSpawner::TankSpawner(UPoint windowSize, std::shared_ptr<int[]> windowBuffer,
                         std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
                         std::shared_ptr<BulletPool> bulletPool)
	: _windowSize{std::move(windowSize)},
	  _windowBuffer{std::move(windowBuffer)},
	  _allObjects{allObjects},
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
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("InitialSpawn", _name, [this]() { InitialSpawn(); });
	_events->AddListener("RespawnTanks", _name, [this]() { RespawnTanks(); });
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_currentMode = newGameMode;
	});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusTank", _name,
			[this](const std::string& author, const std::string& fraction)
			{
				if (fraction == "EnemyTeam")
				{
					IncreaseEnemyRespawnResource();
				}
				else if (fraction == "PlayerTeam")
				{
					if (author == "Player1" || author == "CoopAI1")
					{
						IncreasePlayerOneRespawnResource();
					}
					if (author == "Player2" || author == "CoopAI2")
					{
						IncreasePlayerTwoRespawnResource();
					}
				}
			});

	_events->AddListener("Enemy1_Died", _name, [this]() { _enemyOneNeedRespawn = true; });
	_events->AddListener("Enemy2_Died", _name, [this]() { _enemyTwoNeedRespawn = true; });
	_events->AddListener("Enemy3_Died", _name, [this]() { _enemyThreeNeedRespawn = true; });
	_events->AddListener("Enemy4_Died", _name, [this]() { _enemyFourNeedRespawn = true; });

	_events->AddListener("Player1_Died", _name, [this]() { _playerOneNeedRespawn = true; });
	_events->AddListener("Player2_Died", _name, [this]() { _playerTwoNeedRespawn = true; });
	_events->AddListener("CoopAI1_Died", _name, [this]() { _coopOneAINeedRespawn = true; });
	_events->AddListener("CoopAI2_Died", _name, [this]() { _coopTwoAINeedRespawn = true; });

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
	_events->AddListener("CoopAI1_Spawn", _name, [this]()
	{
		_coopOneAINeedRespawn = false;
		DecreasePlayerOneRespawnResource();
	});
	_events->AddListener("CoopAI2_Spawn", _name, [this]()
	{
		_coopTwoAINeedRespawn = false;
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

void TankSpawner::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("InitialSpawn", _name);
	_events->RemoveListener("RespawnTanks", _name);
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusTank", _name);

	_events->RemoveListener("Enemy1_Died", _name);
	_events->RemoveListener("Enemy2_Died", _name);
	_events->RemoveListener("Enemy3_Died", _name);
	_events->RemoveListener("Enemy4_Died", _name);

	_events->RemoveListener("Player1_Died", _name);
	_events->RemoveListener("Player2_Died", _name);
	_events->RemoveListener("CoopAI1_Died", _name);
	_events->RemoveListener("CoopAI2_Died", _name);

	_events->RemoveListener("Player1_Spawn", _name);
	_events->RemoveListener("Player2_Spawn", _name);
	_events->RemoveListener("CoopAI1_Spawn", _name);
	_events->RemoveListener("CoopAI2_Spawn", _name);

	_events->RemoveListener("Enemy1_Spawn", _name);
	_events->RemoveListener("Enemy2_Spawn", _name);
	_events->RemoveListener("Enemy3_Spawn", _name);
	_events->RemoveListener("Enemy4_Spawn", _name);
}

void TankSpawner::InitialSpawnEnemyTanks(const float gridOffset, const float speed, const int health, const float size)
{
	const bool isNetworkControlled = _currentMode == PlayAsClient;
	SpawnEnemy(1, gridOffset, speed, health, size, isNetworkControlled);
	SpawnEnemy(2, gridOffset, speed, health, size, isNetworkControlled);
	SpawnEnemy(3, gridOffset, speed, health, size, isNetworkControlled);
	SpawnEnemy(4, gridOffset, speed, health, size, isNetworkControlled);
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
	_coopOneAINeedRespawn = false;
	_coopTwoAINeedRespawn = false;
}

void TankSpawner::InitialSpawn()
{
	ResetRespawnStat();

	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	constexpr float speed = 142;
	constexpr int health = 100;
	const float size = gridOffset * 3;

	InitialSpawnPlayerTanks(gridOffset, speed, health, size);

	InitialSpawnEnemyTanks(gridOffset, speed, health, size);
}

void TankSpawner::InitialSpawnPlayerTanks(const float gridOffset, const float speed, const int health, const float size)
{
	if (_currentMode == Demo)
	{
		SpawnCoop1(gridOffset, speed, health, size);
		SpawnCoop2(gridOffset, speed, health, size);

		return;
	}

	if (_currentMode == OnePlayer
	    || _currentMode == TwoPlayers
	    || _currentMode == CoopWithAI
	    || _currentMode == PlayAsHost
	    || _currentMode == PlayAsClient)
	{
		const bool isNetworkControlled = _currentMode == PlayAsClient;
		SpawnPlayer1(gridOffset, speed, health, size, isNetworkControlled);
	}

	if (_currentMode == CoopWithAI)
	{
		SpawnCoop2(gridOffset, speed, health, size);

		return;
	}

	if (_currentMode == TwoPlayers
	    || _currentMode == PlayAsHost
	    || _currentMode == PlayAsClient)
	{
		const bool isSecondNetworkControlled = _currentMode == PlayAsHost || _currentMode == PlayAsClient;
		SpawnPlayer2(gridOffset, speed, health, size, isSecondNetworkControlled);
	}
}

void TankSpawner::SpawnEnemy(const int index, const float gridOffset, const float speed, const int health,
                             const float size, const bool isNetworkControlled)
{
	std::vector<ObjRectangle> spawnPos{
			{.x = gridOffset * 16.f - size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 32.f - size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 16.f + size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 32.f + size * 2.f, .y = 0, .w = size, .h = size}
	};
	for (auto& rect: spawnPos)
	{
		bool isFreeSpawnSpot = true;
		for (const std::shared_ptr<BaseObj>& object: *_allObjects)
		{
			if (ColliderUtils::IsCollide(rect, object->GetShape()))
			{
				isFreeSpawnSpot = false;
			}
		}

		if (isFreeSpawnSpot)
		{
			constexpr int gray = 0x808080;
			std::string name = "Enemy";
			std::string fraction = "EnemyTeam";
			_allObjects->emplace_back(
					std::make_shared<Enemy>(
							rect, gray, health, _windowBuffer, _windowSize, DOWN, speed, _allObjects, _events, name,
							fraction, _bulletPool, isNetworkControlled, index));

			return;
		}
	}
}

void TankSpawner::SpawnPlayer1(const float gridOffset, const float speed, const int health, const float size,
                               const bool isNetworkControlled)
{
	const auto windowSizeY = static_cast<float>(_windowSize.y);
	const ObjRectangle rect{.x = gridOffset * 16.f, .y = windowSizeY - size, .w = size, .h = size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (ColliderUtils::IsCollide(rect, object->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int yellow = 0xeaea00;
		const std::string name = "Player";
		const std::string fraction = "PlayerTeam";

		std::unique_ptr<IInputProvider> inputProvider;
		if (isNetworkControlled)
		{
			inputProvider = std::make_unique<InputProviderForPlayerOneNet>(name, _events);
		}
		else
		{
			inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		}
		_allObjects->emplace_back(
				std::make_shared<PlayerOne>(
						rect, yellow, health, _windowBuffer, _windowSize, UP, speed, _allObjects, _events, name,
						fraction, std::move(inputProvider), _bulletPool, isNetworkControlled, 1));
	}
}

void TankSpawner::SpawnPlayer2(const float gridOffset, const float speed, const int health, const float size,
                               const bool isNetworkControlled)
{
	const auto windowSizeY = static_cast<float>(_windowSize.y);
	const ObjRectangle rect{.x = gridOffset * 32.f, .y = windowSizeY - size, .w = size, .h = size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (ColliderUtils::IsCollide(rect, object->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int green = 0x408000;
		const std::string name = "Player";
		const std::string fraction = "PlayerTeam";
		const bool isHost = _currentMode == PlayAsHost;
		std::unique_ptr<IInputProvider> inputProvider;
		if (isNetworkControlled)
		{
			inputProvider = std::make_unique<InputProviderForPlayerTwoNet>(name, _events);
		}
		else
		{
			inputProvider = std::make_unique<InputProviderForPlayerTwo>(name, _events);
		}
		_allObjects->emplace_back(
				std::make_shared<PlayerTwo>(
						rect, green, health, _windowBuffer, _windowSize, UP, speed, _allObjects, _events, name,
						fraction, std::move(inputProvider), _bulletPool, isNetworkControlled, isHost, 2));
	}
}

void TankSpawner::SpawnCoop1(const float gridOffset, const float speed, const int health, const float size,
                             const bool isNetworkControlled)
{
	const auto windowSizeY = static_cast<float>(_windowSize.y);
	const ObjRectangle rect{.x = gridOffset * 16.f, .y = windowSizeY - size, .w = size, .h = size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (ColliderUtils::IsCollide(rect, object->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int yellow = 0xeaea00;
		std::string name = "CoopAI1";
		std::string fraction = "PlayerTeam";
		_allObjects->emplace_back(
				std::make_shared<CoopAI>(
						rect, yellow, health, _windowBuffer, _windowSize, UP, speed, _allObjects, _events, name,
						fraction, _bulletPool, isNetworkControlled, 1));
	}
}

void TankSpawner::SpawnCoop2(const float gridOffset, const float speed, const int health, const float size,
                             const bool isNetworkControlled)
{
	const auto windowSizeY = static_cast<float>(_windowSize.y);
	const ObjRectangle rect{.x = gridOffset * 32.f, .y = windowSizeY - size, .w = size, .h = size};
	bool isFreeSpawnSpot = true;
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (ColliderUtils::IsCollide(rect, object->GetShape()))
		{
			isFreeSpawnSpot = false;
		}
	}

	if (isFreeSpawnSpot)
	{
		constexpr int green = 0x408000;
		std::string name = "CoopAI2";
		std::string fraction = "PlayerTeam";
		_allObjects->emplace_back(std::make_shared<CoopAI>(
				rect, green, health, _windowBuffer, _windowSize, UP, speed, _allObjects, _events, name, fraction,
				_bulletPool, isNetworkControlled, 2));
	}
}

void TankSpawner::RespawnEnemyTanks(const int index)
{
	if (GetEnemyRespawnResource() > 0)
	{
		const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
		const float size = gridOffset * 3;
		constexpr float speed = 142;
		constexpr int health = 100;
		const bool playAsClient = _currentMode == PlayAsClient;
		SpawnEnemy(index, gridOffset, speed, health, size, playAsClient);
	}
	else
	{
		_enemyOneNeedRespawn = false;
		_events->EmitEvent<const std::string>("ServerSend_TankDied", "Enemy" + std::to_string(index));
	}
}

void TankSpawner::RespawnPlayerTanks(const int index)
{
	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	const float size = gridOffset * 3;
	constexpr float speed = 142;
	constexpr int health = 100;
	const bool playAsClient = _currentMode == PlayAsClient;
	if (index == 1)
	{
		if (GetPlayerOneRespawnResource() > 0)
		{
			SpawnPlayer1(gridOffset, speed, health, size, playAsClient);
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
			SpawnPlayer2(gridOffset, speed, health, size, !playAsClient);
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
	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	const float size = gridOffset * 3;
	constexpr float speed = 142;
	constexpr int health = 100;

	if (index == 1)
	{
		if (GetPlayerOneRespawnResource() > 0)
		{
			SpawnCoop1(gridOffset, speed, health, size);
		}
		else
		{
			_coopOneAINeedRespawn = false;
		}
	}
	else if (index == 2)
	{
		if (GetPlayerTwoRespawnResource() > 0)
		{
			SpawnCoop2(gridOffset, speed, health, size);
		}
		else
		{
			_coopTwoAINeedRespawn = false;
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

	if (IsCoopOneAINeedRespawn()) { RespawnCoopTanks(1); }

	if (IsCoopTwoAINeedRespawn()) { RespawnCoopTanks(2); }
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
