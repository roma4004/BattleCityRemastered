#include "../headers/TankSpawner.h"
#include "../headers/InputProviderForPlayerOne.h"
#include "../headers/InputProviderForPlayerTwo.h"
#include "../headers/pawns/CoopAI.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PlayerOne.h"
#include "../headers/pawns/PlayerTwo.h"
#include "../headers/utils/ColliderUtils.h"

#include <algorithm>
#include <memory>

TankSpawner::TankSpawner(const UPoint windowSize, int* windowBuffer, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                         const std::shared_ptr<EventSystem>& events)
	: _windowSize{windowSize},
	  _windowBuffer{windowBuffer},
	  _allObjects{allObjects},
	  _events{events},
	  _bulletPool{std::make_shared<BulletPool>()}
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
					if (author == "PlayerOne" || author == "CoopOneAI")
					{
						IncreasePlayerOneRespawnResource();
					}
					if (author == "PlayerTwo" || author == "CoopOneTwo")
					{
						IncreasePlayerTwoRespawnResource();
					}
				}
			});

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
		DecreasePlayerOneRespawnResource();
	});
	_events->AddListener("PlayerTwo_Spawn", _name, [this]()
	{
		_playerTwoNeedRespawn = false;
		DecreasePlayerTwoRespawnResource();
	});
	_events->AddListener("CoopOneAI_Spawn", _name, [this]()
	{
		_coopOneAINeedRespawn = false;
		DecreasePlayerOneRespawnResource();
	});
	_events->AddListener("CoopTwoAI_Spawn", _name, [this]()
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
}

void TankSpawner::SpawnEnemyTanks(const float gridOffset, const float speed, const int health, const float size)
{
	SpawnEnemy(1, gridOffset, speed, health, size);
	SpawnEnemy(2, gridOffset, speed, health, size);
	SpawnEnemy(3, gridOffset, speed, health, size);
	SpawnEnemy(4, gridOffset, speed, health, size);
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
		SpawnPlayer1(gridOffset, speed, health, size);
	}

	if (_currentMode == TwoPlayers)
	{
		SpawnPlayer2(gridOffset, speed, health, size);
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
}

void TankSpawner::SpawnEnemy(const int index, const float gridOffset, const float speed, const int health,
                             const float size)
{
	std::vector<Rectangle> spawnPos{
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
			std::string name = "Enemy" + std::to_string(index);
			std::string fraction = "EnemyTeam";
			_allObjects->emplace_back(std::make_shared<Enemy>(rect, gray, health, _windowBuffer, _windowSize, DOWN,
			                                                  speed, _allObjects, _events, name, fraction,
			                                                  _bulletPool));
			return;
		}
	}
}

void TankSpawner::SpawnPlayer1(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 16.f, windowSizeY - size, size, size};
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
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		_allObjects->emplace_back(std::make_shared<PlayerOne>(rect, yellow, health, _windowBuffer, _windowSize, UP,
		                                                      speed, _allObjects, _events, name, fraction,
		                                                      inputProvider, _bulletPool));
	}
}

void TankSpawner::SpawnPlayer2(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 32.f, windowSizeY - size, size, size};
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
		std::string name = "PlayerTwo";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerTwo>(name, _events);
		_allObjects->emplace_back(std::make_shared<PlayerTwo>(rect, green, health, _windowBuffer, _windowSize, UP,
		                                                      speed, _allObjects, _events, name, fraction,
		                                                      inputProvider, _bulletPool));
	}
}

void TankSpawner::SpawnCoop1(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 16.f, windowSizeY - size, size, size};
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
		std::string name = "CoopOneAI";
		std::string fraction = "PlayerTeam";
		_allObjects->emplace_back(std::make_shared<CoopAI>(rect, yellow, health, _windowBuffer, _windowSize, UP, speed,
		                                                   _allObjects, _events, name, fraction, _bulletPool));
	}
}

void TankSpawner::SpawnCoop2(const float gridOffset, const float speed, const int health, const float size)
{
	const float windowSizeY = static_cast<float>(_windowSize.y);
	const Rectangle rect{gridOffset * 32.f, windowSizeY - size, size, size};
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
		std::string name = "CoopTwoAI";
		std::string fraction = "PlayerTeam";
		_allObjects->emplace_back(std::make_shared<CoopAI>(rect, green, health, _windowBuffer, _windowSize, UP, speed,
		                                                   _allObjects, _events, name, fraction, _bulletPool));
	}
}

void TankSpawner::RespawnTanks()
{
	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	const float size = gridOffset * 3;
	constexpr float speed = 142;
	constexpr int health = 100;

	if (IsEnemyOneNeedRespawn() && GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(1, gridOffset, speed, health, size);
	}

	if (IsEnemyTwoNeedRespawn() && GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(2, gridOffset, speed, health, size);
	}

	if (IsEnemyThreeNeedRespawn() && GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(3, gridOffset, speed, health, size);
	}

	if (IsEnemyFourNeedRespawn() && GetEnemyRespawnResource() > 0)
	{
		SpawnEnemy(4, gridOffset, speed, health, size);
	}

	if (IsPlayerOneNeedRespawn() && GetPlayerOneRespawnResource() > 0)
	{
		SpawnPlayer1(gridOffset, speed, health, size);
	}

	if (IsPlayerTwoNeedRespawn() && GetPlayerTwoRespawnResource() > 0)
	{
		SpawnPlayer2(gridOffset, speed, health, size);
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
	_events->EmitEvent<const int>("PlayerOneRespawnResourceChangedTo", _playerOneRespawnResource);
}

void TankSpawner::IncreasePlayerTwoRespawnResource()
{
	++_playerTwoRespawnResource;
	_events->EmitEvent<const int>("PlayerTwoRespawnResourceChangedTo", _playerTwoRespawnResource);
}

void TankSpawner::DecreaseEnemyRespawnResource()
{
	--_enemyRespawnResource;
	_events->EmitEvent<const int>("EnemyRespawnResourceChangedTo", _enemyRespawnResource);
}

void TankSpawner::DecreasePlayerOneRespawnResource()
{
	--_playerOneRespawnResource;
	_events->EmitEvent<const int>("PlayerOneRespawnResourceChangedTo", _playerOneRespawnResource);
}

void TankSpawner::DecreasePlayerTwoRespawnResource()
{
	--_playerTwoRespawnResource;
	_events->EmitEvent<const int>("PlayerTwoRespawnResourceChangedTo", _playerTwoRespawnResource);
}
