#include "../headers/TankSpawner.h"
#include "../headers/GameMode.h"
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

TankSpawner::TankSpawner(const UPoint windowSize, int* windowBuffer, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                         const std::shared_ptr<EventSystem>& events, std::shared_ptr<BulletPool> bulletPool)
	: _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _currentMode{Demo},
	  _allObjects{allObjects},
	  _events{events},
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
			"BonusTank",
			_name,
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

void TankSpawner::SpawnEnemyTanks(const float gridOffset, const float speed, const int health, const float size)
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

	SpawnPlayerTanks(gridOffset, speed, health, size);

	SpawnEnemyTanks(gridOffset, speed, health, size);
}

void TankSpawner::SpawnPlayerTanks(const float gridOffset, const float speed, const int health, const float size)
{
	if (_currentMode == OnePlayer || _currentMode == TwoPlayers || _currentMode == CoopWithAI)
	{
		SpawnPlayer1(gridOffset, speed, health, size, false);
	}

	if (_currentMode == TwoPlayers)
	{
		SpawnPlayer2(gridOffset, speed, health, size, false);
	}

	if (_currentMode == Demo)
	{
		SpawnCoop1(gridOffset, speed, health, size);
		SpawnCoop2(gridOffset, speed, health, size);
	}

	if (_currentMode == CoopWithAI)
	{
		SpawnCoop2(gridOffset, speed, health, size);
	}

	if (_currentMode == PlayAsHost)
	{
		SpawnPlayer1(gridOffset, speed, health, size, false);
		SpawnPlayer2(gridOffset, speed, health, size, true);
	}

	if (_currentMode == PlayAsClient)
	{
		SpawnPlayer1(gridOffset, speed, health, size, true);
		SpawnPlayer2(gridOffset, speed, health, size, true);
	}
}

void TankSpawner::SpawnEnemy(const int index, const float gridOffset, const float speed, const int health,
                             const float size, const bool isNetworkControlled)
{
	std::vector<ObjRectangle> spawnPos{
			{gridOffset * 16.f - size * 2.f, 0, size, size},
			{gridOffset * 32.f - size * 2.f, 0, size, size},
			{gridOffset * 16.f + size * 2.f, 0, size, size},
			{gridOffset * 32.f + size * 2.f, 0, size, size}
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
			_allObjects->emplace_back(std::make_shared<Enemy>(rect, gray, health, _windowBuffer, _windowSize, DOWN,
			                                                  speed, _allObjects, _events, name, fraction, _bulletPool,
			                                                  isNetworkControlled, index));
			return;
		}
	}
}

void TankSpawner::SpawnPlayer1(const float gridOffset, const float speed, const int health, const float size,
                               const bool isNetworkControlled)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const ObjRectangle rect{gridOffset * 16.f, windowSizeY - size, size, size};
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
		std::string name = "Player";
		std::string fraction = "PlayerTeam";

		if (isNetworkControlled)
		{
			std::unique_ptr<IInputProvider> inputProvider =
					std::make_unique<InputProviderForPlayerOneNet>(name, _events);

			_allObjects->emplace_back(std::make_shared<PlayerOne>(rect, yellow, health, _windowBuffer, _windowSize, UP,
			                                                      speed, _allObjects, _events, name, fraction,
			                                                      inputProvider, _bulletPool, isNetworkControlled, 1));
		}
		else
		{
			std::unique_ptr<IInputProvider> inputProvider =
					std::make_unique<InputProviderForPlayerOne>(name, _events);
			_allObjects->emplace_back(std::make_shared<PlayerOne>(rect, yellow, health, _windowBuffer, _windowSize, UP,
			                                                      speed, _allObjects, _events, name, fraction,
			                                                      inputProvider, _bulletPool, isNetworkControlled, 1));
		}
	}
}

void TankSpawner::SpawnPlayer2(const float gridOffset, const float speed, const int health, const float size,
                               const bool isNetworkControlled)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const ObjRectangle rect{gridOffset * 32.f, windowSizeY - size, size, size};
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
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		bool isHost = _currentMode == PlayAsHost;
		if (isNetworkControlled)
		{
			std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerTwoNet>(
					name, _events);
			_allObjects->emplace_back(std::make_shared<PlayerTwo>(rect, green, health, _windowBuffer, _windowSize, UP,
			                                                      speed, _allObjects, _events, name, fraction,
			                                                      inputProvider, _bulletPool, isNetworkControlled,
			                                                      isHost, 2));
		}
		else
		{
			std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerTwo>(
					name, _events);
			_allObjects->emplace_back(std::make_shared<PlayerTwo>(rect, green, health, _windowBuffer, _windowSize, UP,
			                                                      speed, _allObjects, _events, name, fraction,
			                                                      inputProvider, _bulletPool, isNetworkControlled,
			                                                      isHost, 2));
		}
	}
}

void TankSpawner::SpawnCoop1(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const ObjRectangle rect{gridOffset * 16.f, windowSizeY - size, size, size};
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
		_allObjects->emplace_back(std::make_shared<CoopAI>(rect, yellow, health, _windowBuffer, _windowSize, UP, speed,
		                                                   _allObjects, _events, name, fraction, _bulletPool, 1));
	}
}

void TankSpawner::SpawnCoop2(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const ObjRectangle rect{gridOffset * 32.f, windowSizeY - size, size, size};
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
		_allObjects->emplace_back(std::make_shared<CoopAI>(rect, green, health, _windowBuffer, _windowSize, UP, speed,
		                                                   _allObjects, _events, name, fraction, _bulletPool, 2));
	}
}

void TankSpawner::RespawnTanks()
{
	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	const float size = gridOffset * 3;
	constexpr float speed = 142;
	constexpr int health = 100;
	const bool playAsClient = _currentMode == PlayAsClient;

	if (IsEnemyOneNeedRespawn() && GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(1, gridOffset, speed, health, size, playAsClient);
	}

	if (IsEnemyTwoNeedRespawn() && GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(2, gridOffset, speed, health, size, playAsClient);
	}

	if (IsEnemyThreeNeedRespawn() && GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(3, gridOffset, speed, health, size, playAsClient);
	}

	if (IsEnemyFourNeedRespawn() && GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(4, gridOffset, speed, health, size, playAsClient);
	}

	if (IsPlayerOneNeedRespawn() && GetPlayerOneRespawnResource() > 0)
	{
		SpawnPlayer1(gridOffset, speed, health, size, playAsClient);
	}

	if (IsPlayerTwoNeedRespawn() && GetPlayerTwoRespawnResource() > 0)
	{
		SpawnPlayer2(gridOffset, speed, health, size, !playAsClient);
	}

	if (IsCoopOneAINeedRespawn() && GetPlayerOneRespawnResource() > 0)
	{
		SpawnCoop1(gridOffset, speed, health, size);
	}

	if (IsCoopTwoAINeedRespawn() && GetPlayerTwoRespawnResource() > 0)
	{
		SpawnCoop2(gridOffset, speed, health, size);
	}
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
