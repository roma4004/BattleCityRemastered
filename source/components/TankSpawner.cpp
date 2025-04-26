#include "../../headers/components/TankSpawner.h"
#include "../../headers/BaseObjProperty.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/input/InputProviderForPlayerOne.h"
#include "../../headers/input/InputProviderForPlayerOneNet.h"
#include "../../headers/input/InputProviderForPlayerTwo.h"
#include "../../headers/input/InputProviderForPlayerTwoNet.h"
#include "../../headers/pawns/CoopBot.h"
#include "../../headers/pawns/Enemy.h"
#include "../../headers/pawns/PawnProperty.h"
#include "../../headers/pawns/Player.h"
#include "../../headers/utils/ColliderUtils.h"

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

void TankSpawner::SpawnEnemy(const int id, const float speed, const int health)
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
			return ColliderUtils::IsCollide(rect, object->GetShape());
		});

		if (isFreeSpawnSpot)
		{
			constexpr int gray{0x808080};
			std::string name{"Enemy" + std::to_string(id)};
			std::string fraction{"EnemyTeam"};

			BaseObjProperty baseObjProperty{rect, gray, health, true, id, std::move(name), std::move(fraction)};
			PawnProperty pawnProperty{
					std::move(baseObjProperty), _window, DOWN, speed, _allObjects, _events, 1, _gameMode};
			_allObjects->emplace_back(std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool));

			return;
		}
	}
}

void TankSpawner::SpawnPlayer(ObjRectangle rect, const float speed, const int health, const int id)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetShape());
	});

	if (isFreeSpawnSpot)
	{
		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = id == 1 ? yellow : green;
		std::string name{"Player" + std::to_string(id)};//TODO: remove id from name
		std::string fraction{"PlayerTeam"};

		std::unique_ptr<IInputProvider> inputProvider;
		if (id == 1)
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

		BaseObjProperty baseObjProperty{std::move(rect), color, health, true, id, std::move(name), std::move(fraction)};
		PawnProperty pawnProperty{std::move(baseObjProperty), _window, UP, speed, _allObjects, _events, 1, _gameMode};
		_allObjects->emplace_back(
				std::make_shared<Player>(std::move(pawnProperty), _bulletPool, std::move(inputProvider)));
	}
}

void TankSpawner::SpawnCoopBot(ObjRectangle rect, const float speed, const int health, const int id)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetShape());
	});

	if (isFreeSpawnSpot)
	{
		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = id == 1 ? yellow : green;
		std::string name{"CoopBot" + std::to_string(id)};
		std::string fraction{"PlayerTeam"};

		BaseObjProperty baseObjProperty{std::move(rect), color, health, true, id, std::move(name), std::move(fraction)};
		PawnProperty pawnProperty{std::move(baseObjProperty), _window, UP, speed, _allObjects, _events, 1, _gameMode};
		_allObjects->emplace_back(std::make_shared<CoopBot>(std::move(pawnProperty), _bulletPool));
	}
}

void TankSpawner::RespawnEnemyTanks(const int id)
{
	if (_enemyRespawnResource > 0)
	{
		constexpr float speed{142};
		constexpr int health{100};
		SpawnEnemy(id, speed, health);
	}
	else
	{
		_enemyOneNeedRespawn = false;
		_enemyTwoNeedRespawn = false;
		_enemyThreeNeedRespawn = false;
		_enemyFourNeedRespawn = false;
	}
}

void TankSpawner::RespawnPlayerTanks(const int id)
{
	const float windowSizeY{static_cast<float>(_window->size.y)};
	const float gridOffset{windowSizeY / 50.f};
	const float size{gridOffset * 3};
	constexpr float speed{142};
	constexpr int health{100};

	if (id == 1)
	{
		if (_playerOneRespawnResource > 0)
		{
			ObjRectangle rect{.x = gridOffset * 16.f, .y = windowSizeY - size, .w = size, .h = size};
			SpawnPlayer(std::move(rect), speed, health, id);
		}
		else
		{
			_playerOneNeedRespawn = false;
		}
	}
	else
	{
		if (_playerTwoRespawnResource > 0)
		{
			ObjRectangle rect{.x = gridOffset * 32.f, .y = windowSizeY - size, .w = size, .h = size};
			SpawnPlayer(std::move(rect), speed, health, id);
		}
		else
		{
			_playerTwoNeedRespawn = false;
		}
	}
}

void TankSpawner::RespawnCoopTanks(const int id)
{
	const float windowSizeY{static_cast<float>(_window->size.y)};
	const float gridOffset{windowSizeY / 50.f};
	const float size{gridOffset * 3};
	constexpr float speed{142};
	constexpr int health{100};

	if (id == 1)
	{
		if (_playerOneRespawnResource > 0)
		{
			ObjRectangle rect{.x = gridOffset * 16.f, .y = windowSizeY - size, .w = size, .h = size};
			SpawnCoopBot(std::move(rect), speed, health, id);
		}
		else
		{
			_coopBotOneNeedRespawn = false;
		}
	}
	else
	{
		if (_playerTwoRespawnResource > 0)
		{
			ObjRectangle rect{.x = gridOffset * 32.f, .y = windowSizeY - size, .w = size, .h = size};
			SpawnCoopBot(std::move(rect), speed, health, id);
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
		if (author == "Player2" || author == "CoopBot2")
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

void TankSpawner::OnTankDied(const std::string& whoDied)
{
	if (whoDied == "Enemy1")
	{
		this->_enemyOneNeedRespawn = true;
	}
	else if (whoDied == "Enemy2")
	{
		this->_enemyTwoNeedRespawn = true;
	}
	else if (whoDied == "Enemy3")
	{
		this->_enemyThreeNeedRespawn = true;
	}
	else if (whoDied == "Enemy4")
	{
		this->_enemyFourNeedRespawn = true;
	}
	else if (whoDied == "Player1")
	{
		this->_playerOneNeedRespawn = true;
	}
	else if (whoDied == "Player2")
	{
		this->_playerTwoNeedRespawn = true;
	}
	else if (whoDied == "CoopBot1")
	{
		this->_coopBotOneNeedRespawn = true;
	}
	else if (whoDied == "CoopBot2")
	{
		this->_coopBotTwoNeedRespawn = true;
	}
}
