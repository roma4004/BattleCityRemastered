#include "../../headers/components/TankSpawner.h"
#include "../../headers/BaseObjProperty.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/enums/RespawnResource.h"
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

	_slots = {
			{uuidTankGenerator(), false},
			{uuidTankGenerator(), false},
			{uuidTankGenerator(), false},
			{uuidTankGenerator(), false},
			{uuidTankGenerator(), false},
			{uuidTankGenerator(), false},
	};

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
	_events->AddListener<const buuid&>("TankSpawn", _name, [this](const buuid& uuid)
	{
		OnTankSpawn(uuid);
	});
	_events->AddListener<const buuid&>("TankDied", _name, [this](const buuid& uuid)
	{
		OnTankDied(uuid);
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

	_events->AddListener<const TankType, const buuid&>(
			"ClientReceived_RespawnTank", _name, [this](const TankType type, const buuid& uuid)
			{
				this->RespawnClient(type, uuid);
			});
}

void TankSpawner::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("RespawnTanks", _name);
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);
	_events->RemoveListener<const buuid&>("TankSpawn", _name);
	_events->RemoveListener<const buuid&>("TankDied", _name);

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
	for (int i = 0; i < 4; ++i)
	{
		_slots[i].isAvailable = true;
	}
}

//NOTE: use only in unit tests
void TankSpawner::SetSlotNeedRespawn(const int slotIndex)
{
	if (slotIndex >= 0 && slotIndex < 6)
	{
		_slots[slotIndex].isAvailable = true;
	}
}

void TankSpawner::ResetRespawnStat()
{
	_respawnResource[ENEMY_ALL] = 20;
	_respawnResource[PLAYER_ONE] = 3;
	_respawnResource[PLAYER_TWO] = 3;

	for (SpawnSlot& slot: _slots)
	{
		slot.isAvailable = false;
	}
}

void TankSpawner::ResetSpawn()
{
	ResetRespawnStat();

	SetPlayerNeedRespawn();

	SetEnemyNeedRespawn();
}

void TankSpawner::SetPlayerNeedRespawn()
{
	_slots[4].isAvailable = true;

	if (_gameMode != OnePlayer)
	{
		_slots[5].isAvailable = true;
	}
}

std::string TankSpawner::GetCurrentTimeString()
{
	const auto now = std::chrono::system_clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(now);
	const auto ms = std::chrono::duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

	std::tm timeInfo;
	localtime_s(&timeInfo, &nowTime);

	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%H:%M:%S") << '.'
			<< std::setfill('0') << std::setw(3) << ms.count();

	return ss.str();
}

void TankSpawner::SpawnEnemy(const buuid uuid, const float speed, const int health)
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
			if (object.get() == nullptr)
			{
				return false;
			}

			return ColliderUtils::IsCollide(rect, object->GetRect());
		});

		if (isFreeSpawnSpot)
		{
			std::string fraction{"EnemyTeam"};
			std::string name{"Enemy"};

			// Log enemy tank spawn
			Logger::GetInstance().LogTankSpawn(name, fraction, boost::uuids::to_string(uuid));
			std::cout << "[" << GetCurrentTimeString() << "] "
					<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
					<< "SpawnEnemy  UUID = " << uuid
					<< ", Name = " << name
					<< std::endl;

			constexpr int gray{0x808080};
			RespawnTank<Enemy>(std::move(rect), gray, health, std::move(name), std::move(fraction), speed,
			                   std::move(uuid));

			return;
		}
	}
}

void TankSpawner::SpawnPlayer(ObjRectangle rect, const float speed, const int health, const buuid uuid,
                              const TankType type)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		if (object.get() == nullptr)
		{
			return false;
		}

		return ColliderUtils::IsCollide(rect, object->GetRect());
	});

	if (isFreeSpawnSpot)
	{
		std::string name{type == PLAYER1 ? "Player1" : "Player2"};
		std::string fraction{"PlayerTeam"};

		// Log tank spawn
		Logger::GetInstance().LogTankSpawn(name, fraction, boost::uuids::to_string(uuid));
		std::cout << "[" << GetCurrentTimeString() << "] "
				<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
				<< "SpawnPlayer UUID = " << uuid
				<< ", Name = " << name
				<< std::endl;

		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = type == PLAYER1 ? yellow : green;
		RespawnTank<Player>(std::move(rect), color, health, std::move(name), std::move(fraction), speed,
		                    std::move(uuid));
	}
}

void TankSpawner::SpawnCoopBot(ObjRectangle rect, const float speed, const int health, const buuid uuid,
                               const TankType type)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		if (object.get() == nullptr)
		{
			return false;
		}

		return ColliderUtils::IsCollide(rect, object->GetRect());
	});

	if (isFreeSpawnSpot)
	{
		std::string name{(type == COOP1 ? "CoopBot1" : "CoopBot2")};
		std::string fraction{"PlayerTeam"};

		// Log coop bot spawn
		Logger::GetInstance().LogTankSpawn(name, fraction, boost::uuids::to_string(uuid));
		std::cout << "[" << GetCurrentTimeString() << "] "
				<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
				<< "SpawnEnemy  UUID = " << uuid
				<< ", Name = " << name
				<< std::endl;

		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = type == COOP1 ? yellow : green;
		RespawnTank<CoopBot>(std::move(rect), color, health, std::move(name), std::move(fraction), speed,
		                     std::move(uuid));
	}
}

void TankSpawner::RespawnEnemyTanks(const TankType type, const buuid uuid)
{
	constexpr float speed{142};
	constexpr int health{100};
	SpawnEnemy(uuid, speed, health);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const TankType, const buuid&>("ServerSend_RespawnTank", type, uuid);
	}
}

void TankSpawner::RespawnPlayerTeam(const TankType type, const buuid uuid)
{
	const float windowSizeY{static_cast<float>(_window->size.y)};
	const float gridOffset{windowSizeY / 50.f};
	const float size{gridOffset * 3};
	constexpr float speed{142};
	constexpr int health{100};
	const bool isFirst = type == PLAYER1;
	ObjRectangle rect{.x = gridOffset * (isFirst ? 16.f : 32.f), .y = windowSizeY - size, .w = size, .h = size};

	if (_gameMode == OnePlayer
	    || _gameMode == TwoPlayers
	    || _gameMode == PlayAsHost
	    || _gameMode == PlayAsClient
	    || (_gameMode == CoopWithBot && isFirst))
	{
		SpawnPlayer(std::move(rect), speed, health, uuid, type);
	}
	else if (_gameMode == Demo || _gameMode == CoopWithBot)
	{
		SpawnCoopBot(std::move(rect), speed, health, uuid, isFirst ? COOP1 : COOP2);
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const TankType, const buuid&>("ServerSend_RespawnTank", type, uuid);
	}
}

void TankSpawner::RespawnTanks()
{
	for (int i = 0; i < _slots.size(); ++i)
	{
		if (_slots[i].isAvailable)
		{
			switch (i)
			{
				case TankType::ENEMY:
				case 1:
				case 2:
				case 3:
					RespawnEnemyTanks(static_cast<TankType>(i), _slots[i].id);
					break;
				case TankType::PLAYER1:
				case TankType::PLAYER2:
					RespawnPlayerTeam(static_cast<TankType>(i), _slots[i].id);
					break;
				default:
					break;
			}
			_slots[i].isAvailable = false;
			break;
		}
	}
}

void TankSpawner::RespawnClient(const TankType type, const buuid uuid)
{
	switch (type)
	{
		case TankType::ENEMY:
		case 1:
		case 2:
		case 3:
			RespawnEnemyTanks(type, uuid);
			break;
		case TankType::PLAYER1:
		case TankType::PLAYER2:
			RespawnPlayerTeam(type, uuid);
			break;
		default:
			break;
	}
}

void TankSpawner::IncreaseEnemyRespawnResource()
{
	++_respawnResource[ENEMY_ALL];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Enemy",
	                                                  _respawnResource[ENEMY_ALL]);
}

void TankSpawner::IncreasePlayerOneRespawnResource()
{
	++_respawnResource[PLAYER_ONE];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player1",
	                                                  _respawnResource[PLAYER_ONE]);
}

void TankSpawner::IncreasePlayerTwoRespawnResource()
{
	++_respawnResource[PLAYER_TWO];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player2",
	                                                  _respawnResource[PLAYER_TWO]);
}

void TankSpawner::DecreaseEnemyRespawnResource()
{
	--_respawnResource[ENEMY_ALL];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Enemy",
	                                                  _respawnResource[ENEMY_ALL]);
}

void TankSpawner::DecreasePlayerOneRespawnResource()
{
	--_respawnResource[PLAYER_ONE];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player1",
	                                                  _respawnResource[PLAYER_ONE]);
}

void TankSpawner::DecreasePlayerTwoRespawnResource()
{
	--_respawnResource[PLAYER_TWO];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player2",
	                                                  _respawnResource[PLAYER_TWO]);
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

void TankSpawner::OnTankSpawn(const buuid& uuid)
{
	for (int i = 0; i < _slots.size(); ++i)
	{
		if (_slots[i].id == uuid)
		{
			switch (i)
			{
				case TankType::ENEMY:
				case 1:
				case 2:
				case 3:
					DecreaseEnemyRespawnResource();
					break;
				case TankType::PLAYER1:
					DecreasePlayerOneRespawnResource();
					break;
				case TankType::PLAYER2:
					DecreasePlayerTwoRespawnResource();
					break;
				default:
					break;
			}
			_slots[i].isAvailable = false;
			break;
		}
	}
}

void TankSpawner::OnTankDied(const buuid& uuid)
{
	//TODO: replace with std:: algorithm
	for (int i = 0; i < _slots.size(); ++i)
	{
		if (_slots[i].id == uuid)
		{
			switch (i)
			{
				case TankType::ENEMY:
				case 1:
				case 2:
				case 3:
					_slots[i].isAvailable = _respawnResource[ENEMY_ALL] > 0;
					break;
				case TankType::PLAYER1:
					_slots[i].isAvailable = _respawnResource[PLAYER_ONE] > 0;
					break;
				case TankType::PLAYER2:
					_slots[i].isAvailable = _respawnResource[PLAYER_TWO] > 0;
					break;
				default:
					break;
			}
			break;
		}
	}
}
