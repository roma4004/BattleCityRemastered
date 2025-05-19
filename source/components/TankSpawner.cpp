#include "../../headers/components/TankSpawner.h"
#include "../../headers/BaseObjProperty.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/enums/TankType.h"
#include "../../headers/input/InputProviderForPlayerOne.h"
#include "../../headers/input/InputProviderForPlayerOneNet.h"
#include "../../headers/input/InputProviderForPlayerTwo.h"
#include "../../headers/input/InputProviderForPlayerTwoNet.h"
#include "../../headers/pawns/CoopBot.h"
#include "../../headers/pawns/Enemy.h"
#include "../../headers/pawns/PawnProperty.h"
#include "../../headers/pawns/Player.h"
#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/utils/Logger.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

TankSpawner::TankSpawner(std::shared_ptr<Window> window, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                         std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool)
	: _allObjects{allObjects},
	  _window{std::move(window)},
	  _events{std::move(events)},
	  _bulletPool{std::move(bulletPool)}
{
	static boost::uuids::random_generator uuidTankGenerator;

	_enemyOneUuid = uuidTankGenerator();
	_enemyTwoUuid = uuidTankGenerator();
	_enemyThreeUuid = uuidTankGenerator();
	_enemyFourUuid = uuidTankGenerator();
	_playerOneUuid = uuidTankGenerator();
	_playerTwoUuid = uuidTankGenerator();

	Subscribe();
}

TankSpawner::~TankSpawner()
{
	Unsubscribe();
}

void TankSpawner::Subscribe()
{
	//TODO: reuse existing tanks when gamemode changed
	//TODO: need work phase, clearState (all spawns disabled), battleState (spawn as normal)
	_events->AddListener("Reset", _name, [this]() { ResetSpawn(); });
	_events->AddListener("RespawnTanks", _name, [this]()
	{
		RespawnTanks();
	});
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		_gameMode == PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
	});
	_events->AddListener<const std::string&>("TankSpawn", _name, [this](const std::string& whoSpawn)
	{
		OnTankSpawn(whoSpawn);
	});
	_events->AddListener<const std::string&>("TankDied", _name, [this](const std::string& whoDied)
	{
		OnTankDied(whoDied);
	});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusTank", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusTank(author, fraction);
			});
}

void TankSpawner::SubscribeAsClient()
{
	_events->AddListener<const std::string&, const std::string&>(
			"ClientReceived_OnTank", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusTank(author, fraction);
			});

	_events->AddListener<const std::string&, const std::string&>(
			"ClientReceived_OnGrenade", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusGrenade(author, fraction);
			});

	_events->AddListener<const TankType, const boost::uuids::uuid>(
			"ClientReceived_RespawnTank", _name, [this](const TankType type, const boost::uuids::uuid uuid)
			{
				this->RespawnClient(type, uuid);
			});
}

void TankSpawner::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("RespawnTanks", _name);
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);
	_events->RemoveListener<const std::string&>("TankSpawn", _name);
	_events->RemoveListener<const std::string&>("TankDied", _name);

	if (_gameMode == PlayAsClient)
	{
		UnsubscribeAsClient();
	}

	_events->RemoveListener<const std::string&, const std::string&>("BonusTank", _name);
}

void TankSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener<const std::string&, const std::string&>("ClientReceived_" + _name + "OnTank", _name);
	_events->RemoveListener<const std::string&, const std::string&>("ClientReceived_OnGrenade", _name);
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
	if (_gameMode == Demo)
	{
		_coopBotOneNeedRespawn = true;
		_coopBotTwoNeedRespawn = true;

		return;
	}

	_playerOneNeedRespawn = true;

	if (_gameMode == CoopWithBot)
	{
		_coopBotTwoNeedRespawn = true;

		return;
	}

	if (_gameMode == TwoPlayers
	    || _gameMode == PlayAsHost
	    || _gameMode == PlayAsClient)
	{
		_playerTwoNeedRespawn = true;
	}
}

std::string TankSpawner::GetCurrentTimeString()
{
	const auto now = std::chrono::system_clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(now);
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

	std::tm timeInfo;
	localtime_s(&timeInfo, &nowTime);

	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%H:%M:%S") << '.'
			<< std::setfill('0') << std::setw(3) << ms.count();

	return ss.str();
}

void TankSpawner::SpawnEnemy(const boost::uuids::uuid uuid, const float speed, const int health, const TankType type)
{
	const float gridOffset{static_cast<float>(_window->size.y) / 50.f};
	const float size{gridOffset * 3};
	const static std::vector<ObjRectangle> spawnPos{{
			{.x = gridOffset * 16.f - size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 32.f - size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 16.f + size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 32.f + size * 2.f, .y = 0, .w = size, .h = size}
	}};

	for (const auto& rect: spawnPos)
	{
		const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
		{
			return ColliderUtils::IsCollide(rect, object->GetRect());
		});

		if (isFreeSpawnSpot)
		{
			constexpr int gray{0x808080};
			std::string fraction{"EnemyTeam"};
			std::string name{"Enemy"};
			if (type == ENEMY1)
			{
				name += "1";
			}
			else if (type == ENEMY2)
			{
				name += "2";
			}
			else if (type == ENEMY3)
			{
				name += "3";
			}
			else if (type == ENEMY4)
			{
				name += "4";
			}

			// Log enemy tank spawn
			Logger::GetInstance().LogTankSpawn(name, fraction, boost::uuids::to_string(uuid));
			std::cout << "[" << GetCurrentTimeString() << "] "
			<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
			<< "SpawnEnemy  UUID = " << uuid
			<< ", Name = " << name
			<< std::endl;

			BaseObjProperty baseObjProperty{rect, gray, health, true, uuid, std::move(name), std::move(fraction)};
			PawnProperty pawnProperty{
					std::move(baseObjProperty), _window, UP, speed, _allObjects, _events, 1, _gameMode};
			_allObjects->emplace_back(std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool));

			return;
		}
	}
}

void TankSpawner::SpawnPlayer(ObjRectangle rect, const float speed, const int health, const boost::uuids::uuid uuid,
                              const TankType type)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetRect());
	});

	if (isFreeSpawnSpot)
	{
		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = type == PLAYER1 ? yellow : green;
		std::string name{type == PLAYER1 ? "Player1" : "Player2"};
		std::string fraction{"PlayerTeam"};

		std::unique_ptr<IInputProvider> inputProvider;
		if (type == PLAYER1)
		{
			if (_gameMode == PlayAsClient)
			{
				inputProvider = std::make_unique<InputProviderForPlayerOneNet>(_events);
			}
			else
			{
				inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);
			}
		}
		else
		{
			if (_gameMode == PlayAsClient || _gameMode == PlayAsHost)
			{
				inputProvider = std::make_unique<InputProviderForPlayerTwoNet>(_events);
			}
			else
			{
				inputProvider = std::make_unique<InputProviderForPlayerTwo>(_events);
			}
		}

		// Log tank spawn
		Logger::GetInstance().LogTankSpawn(name, fraction, boost::uuids::to_string(uuid));
		std::cout << "[" << GetCurrentTimeString() << "] "
		<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
		<< "SpawnPlayer UUID = " << uuid
		<< ", Name = " << name
		<< std::endl;

		BaseObjProperty baseObjProperty{std::move(rect), color, health, true, uuid, std::move(name),
		                                std::move(fraction)};
		PawnProperty pawnProperty{std::move(baseObjProperty), _window, UP, speed, _allObjects, _events, 1, _gameMode};
		_allObjects->emplace_back(
				std::make_shared<Player>(std::move(pawnProperty), _bulletPool, std::move(inputProvider)));
	}
}

void TankSpawner::SpawnCoopBot(ObjRectangle rect, const float speed, const int health, const boost::uuids::uuid uuid,
                               const TankType type)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetRect());
	});

	if (isFreeSpawnSpot)
	{
		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = type == COOP1 ? yellow : green;
		std::string name{(type == COOP1 ? "CoopBot1" : "CoopBot2")};
		std::string fraction{"PlayerTeam"};

		// Log coop bot spawn
		Logger::GetInstance().LogTankSpawn(name, fraction, boost::uuids::to_string(uuid));
		std::cout << "[" << GetCurrentTimeString() << "] "
		<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
		<< "SpawnEnemy  UUID = " << uuid
		<< ", Name = " << name
		<< std::endl;

		BaseObjProperty baseObjProperty{std::move(rect), color, health, true, uuid, std::move(name),
		                                std::move(fraction)};
		PawnProperty pawnProperty{std::move(baseObjProperty), _window, UP, speed, _allObjects, _events, 1, _gameMode};
		_allObjects->emplace_back(std::make_shared<CoopBot>(std::move(pawnProperty), _bulletPool));
	}
}

void TankSpawner::RespawnEnemyTanks(const TankType type, const boost::uuids::uuid uuid)
{
	constexpr float speed{142};
	constexpr int health{100};
	SpawnEnemy(uuid, speed, health, type);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const TankType, const boost::uuids::uuid>("ServerSend_RespawnTank", type, uuid);
	}
}

void TankSpawner::RespawnPlayerTeam(const TankType type, const boost::uuids::uuid uuid)
{
	const float windowSizeY{static_cast<float>(_window->size.y)};
	const float gridOffset{windowSizeY / 50.f};
	const float size{gridOffset * 3};
	constexpr float speed{142};
	constexpr int health{100};
	const bool isFirst = type == PLAYER1 || type == COOP1;
	ObjRectangle rect{.x = gridOffset * (isFirst ? 16.f : 32.f), .y = windowSizeY - size, .w = size, .h = size};

	if (type == PLAYER1 || type == PLAYER2)
	{
		SpawnPlayer(std::move(rect), speed, health, uuid, type);
	}
	else if (type == COOP1 || type == COOP2)
	{
		SpawnCoopBot(std::move(rect), speed, health, uuid, type);
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const TankType, const boost::uuids::uuid>("ServerSend_RespawnTank", type, uuid);
	}
}

void TankSpawner::RespawnTanks()
{
	if (IsEnemyOneNeedRespawn()) { RespawnEnemyTanks(ENEMY1, _enemyOneUuid); }//TODO: check handle if already spawned

	if (IsEnemyTwoNeedRespawn()) { RespawnEnemyTanks(ENEMY2, _enemyTwoUuid); }

	if (IsEnemyThreeNeedRespawn()) { RespawnEnemyTanks(ENEMY3, _enemyThreeUuid); }

	if (IsEnemyFourNeedRespawn()) { RespawnEnemyTanks(ENEMY4, _enemyFourUuid); }


	if (IsPlayerOneNeedRespawn()) { RespawnPlayerTeam(PLAYER1, _playerOneUuid); }

	if (IsPlayerTwoNeedRespawn()) { RespawnPlayerTeam(PLAYER2, _playerTwoUuid); }


	if (IsCoopBotOneNeedRespawn()) { RespawnPlayerTeam(COOP1, _playerOneUuid); }

	if (IsCoopBotTwoNeedRespawn()) { RespawnPlayerTeam(COOP2, _playerTwoUuid); }
}

void TankSpawner::RespawnClient(const TankType type, const boost::uuids::uuid uuid)
{
	if (type == ENEMY1 || type == ENEMY2 || type == ENEMY3 || type == ENEMY4)
	{
		RespawnEnemyTanks(type, uuid);
	}

	if (type == PLAYER1 || type == PLAYER2)
	{
		RespawnPlayerTeam(type, uuid);
	}
}

void TankSpawner::IncreaseEnemyRespawnResource()
{
	++_enemyRespawnResource;
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Enemy", _enemyRespawnResource);
}

void TankSpawner::IncreasePlayerOneRespawnResource()
{
	++_playerOneRespawnResource;
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player1", _playerOneRespawnResource);
}

void TankSpawner::IncreasePlayerTwoRespawnResource()
{
	++_playerTwoRespawnResource;
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player2", _playerTwoRespawnResource);
}

void TankSpawner::DecreaseEnemyRespawnResource()
{
	--_enemyRespawnResource;
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Enemy", _enemyRespawnResource);
}

void TankSpawner::DecreasePlayerOneRespawnResource()
{
	--_playerOneRespawnResource;
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player1", _playerOneRespawnResource);
}

void TankSpawner::DecreasePlayerTwoRespawnResource()
{
	--_playerTwoRespawnResource;
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player2", _playerTwoRespawnResource);
}

void TankSpawner::OnBonusGrenade(const std::string& author, const std::string& fraction)
{
	if (fraction == "PlayerTeam")
	{
		if (author == "Player1")
		{
			DecreasePlayerOneRespawnResource();
		}
		else if (author == "Player2")
		{
			DecreasePlayerTwoRespawnResource();
		}
	}
	else if (fraction == "EnemyTeam")
	{
		DecreaseEnemyRespawnResource();
	}
}

void TankSpawner::OnBonusTank(const std::string& author, const std::string& fraction)
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
		else if (author == "Player2" || author == "CoopBot2")
		{
			IncreasePlayerTwoRespawnResource();
		}
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&>("ServerSend_OnTank", author, fraction);
	}
}

void TankSpawner::OnTankSpawn(const std::string& whoSpawn)
{
	if (whoSpawn == "Enemy1")
	{
		_enemyOneNeedRespawn = false;
		DecreaseEnemyRespawnResource();
	}
	else if (whoSpawn == "Enemy2")
	{
		_enemyTwoNeedRespawn = false;
		DecreaseEnemyRespawnResource();
	}
	else if (whoSpawn == "Enemy3")
	{
		_enemyThreeNeedRespawn = false;
		DecreaseEnemyRespawnResource();
	}
	else if (whoSpawn == "Enemy4")
	{
		_enemyFourNeedRespawn = false;
		DecreaseEnemyRespawnResource();
	}
	else if (whoSpawn == "Player1")
	{
		_playerOneNeedRespawn = false;
		DecreasePlayerOneRespawnResource();
	}
	else if (whoSpawn == "Player2")
	{
		_playerTwoNeedRespawn = false;
		DecreasePlayerTwoRespawnResource();
	}
	else if (whoSpawn == "CoopBot1")
	{
		_coopBotOneNeedRespawn = false;
		DecreasePlayerOneRespawnResource();
	}
	else if (whoSpawn == "CoopBot2")
	{
		_coopBotTwoNeedRespawn = false;
		DecreasePlayerTwoRespawnResource();
	}
}

void TankSpawner::OnEnemyTankDied(const std::string& whoDied)
{
	if (_enemyRespawnResource > 0)
	{
		if (whoDied == "Enemy1")
		{
			_enemyOneNeedRespawn = true;
		}
		else if (whoDied == "Enemy2")
		{
			_enemyTwoNeedRespawn = true;
		}
		else if (whoDied == "Enemy3")
		{
			_enemyThreeNeedRespawn = true;
		}
		else if (whoDied == "Enemy4")
		{
			_enemyFourNeedRespawn = true;
		}
	}
	else
	{
		_enemyOneNeedRespawn = false;
		_enemyTwoNeedRespawn = false;
		_enemyThreeNeedRespawn = false;
		_enemyFourNeedRespawn = false;
	}
}

void TankSpawner::OnPlayerTankDied(const std::string& whoDied)
{
	if (_playerOneRespawnResource > 0)
	{
		if (whoDied == "Player1")
		{
			_playerOneNeedRespawn = true;
		}
		else if (whoDied == "CoopBot1")
		{
			_coopBotOneNeedRespawn = true;
		}
	}
	else
	{
		_playerOneNeedRespawn = false;
		_coopBotOneNeedRespawn = false;
	}

	if (_playerTwoRespawnResource > 0)
	{
		if (whoDied == "Player2")
		{
			_playerTwoNeedRespawn = true;
		}
		else if (whoDied == "CoopBot2")
		{
			_coopBotTwoNeedRespawn = true;
		}
	}
	else
	{
		_playerTwoNeedRespawn = false;
		_coopBotTwoNeedRespawn = false;
	}
}

void TankSpawner::OnTankDied(const std::string& whoDied)
{
	OnEnemyTankDied(whoDied);

	OnPlayerTankDied(whoDied);
}
