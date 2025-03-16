#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/EventSystem.h"
#include "../../headers/Pawns/Pawn.h"
#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <string>

FortressWall::FortressWall(const ObjRectangle& rect, std::shared_ptr<Window> window,
                           const std::shared_ptr<EventSystem>& events,
                           std::vector<std::shared_ptr<BaseObj>>* allObjects, const int id, const GameMode gameMode)
	: BaseObj{rect, 0x924b00, 1, id, "fortressWall" + std::to_string(id), "Neutral"},
	  _rect{rect},
	  _gameMode{gameMode},
	  _window{window},//TODO: change name for statistic
	  _events{events},
	  _allObjects{allObjects},
	  _obstacle{std::make_unique<BrickWall>(rect, window, events, id, gameMode)}
{
	Subscribe();
}

FortressWall::~FortressWall()
{
	Unsubscribe();
}

void FortressWall::Subscribe()
{
	_gameMode == PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();

	SubscribeBonus();
}

void FortressWall::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void FortressWall::SubscribeAsClient()
{
	_events->AddListener<const int>("ClientReceived_FortressDied", _name, [this](const int id)
	{
		if (id != _id)
		{
			return;
		}

		this->OnEnemyPickupShovel();
	});

	_events->AddListener<const int>("ClientReceived_FortressToSteel", _name, [this](const int id)
	{
		if (id == _id)
		{
			this->OnPlayerPickupShovel();
		}
	});

	_events->AddListener<const int>("ClientReceived_FortressToBrick", _name, [this](const int id)
	{
		if (id == _id)
		{
			this->OnPlayerShovelCooldownEnd();
		}
	});
}

void FortressWall::SubscribeBonus()
{
	_events->AddListener<const std::string&, const std::string&, std::chrono::milliseconds>(
			"BonusShovel", _name,//TODO: remove author when dont needed in all event
			[this](const std::string& /*author*/, const std::string& fraction,
			       const std::chrono::milliseconds bonusDurationTime)
			{
				if (fraction == "PlayerTeam")
				{
					if (this->_shovel.isActive)
					{
						this->_shovel.cooldown += bonusDurationTime;

						return;
					}

					this->_shovel.isActive = true;
					this->OnPlayerPickupShovel();
					this->_shovel.cooldown = bonusDurationTime;
				}
				else if (fraction == "EnemyTeam")
				{
					this->_shovel.isActive = false;
					this->OnEnemyPickupShovel();
				}

				this->_shovel.activateTime = std::chrono::system_clock::now();
			});
}

void FortressWall::Unsubscribe() const
{
	_gameMode == PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();

	UnsubscribeBonus();
}

void FortressWall::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _name);
}

void FortressWall::UnsubscribeAsClient() const
{
	_events->RemoveListener<const int>("ClientReceived_FortressDied", _name);
	_events->RemoveListener<const int>("ClientReceived_FortressToSteel", _name);
	_events->RemoveListener<const int>("ClientReceived_FortressToBrick", _name);
}

void FortressWall::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const std::string&, std::chrono::milliseconds>("BonusShovel", _name);
}

void FortressWall::Draw() const {}

void FortressWall::TickUpdate(const float /*deltaTime*/)
{
	// bonus disable timer
	if (_shovel.isActive && TimeUtils::IsCooldownFinish(_shovel.activateTime, _shovel.cooldown))
	{
		_shovel.isActive = false;
		OnPlayerShovelCooldownEnd();
	}
}

void FortressWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (std::holds_alternative<std::unique_ptr<BrickWall>>(_obstacle))
	{
		_events->EmitEvent<const std::string&, const std::string&>("BrickWallDied", author, fraction);//for statistic
	}
	else
	{
		_events->EmitEvent<const std::string&, const std::string&>("SteelWallDied", author, fraction);//for statistic
	}
}

void FortressWall::OnPlayerShovelCooldownEnd()
{
	if (GetHealth() > 0)
	{
		if (std::holds_alternative<std::unique_ptr<SteelWall>>(_obstacle))
		{
			_obstacle = std::make_unique<BrickWall>(_rect, _window, _events, _id, _gameMode);
		}

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const int>("ServerSend_FortressToBrick", _id);
		}
	}
}

//TODO: should be private and friend bonusShovel to activate
// NOTE: call when player team bonus pick up
void FortressWall::OnPlayerPickupShovel()
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [this](const std::shared_ptr<BaseObj>& object)
	{
		if (dynamic_cast<Pawn*>(object.get()))
		{
			return ColliderUtils::IsCollide(_rect, object->GetShape());
		}

		return false;
	});

	if (isFreeSpawnSpot)//Check if neared tank/bullet/bonus suppressed this spawn
	{
		_obstacle = std::make_unique<SteelWall>(_rect, _window, _events, _id, _gameMode);

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const int>("ServerSend_FortressToSteel", _id);
		}
	}
}

void FortressWall::TakeDamage(const int damage)
{
	int health{0};

	std::visit([damage, &health](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			auto* obstacle = uniqPtr.get();
			obstacle->TakeDamage(damage);
			health = obstacle->GetHealth();
		}
	}, _obstacle);

	if (health <= 0)
	{
		_obstacle = std::unique_ptr<BrickWall>(nullptr);

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const int>("ServerSend_FortressDied", _id);
		}
	}
}

bool FortressWall::IsBrickWall() const
{
	return std::holds_alternative<std::unique_ptr<BrickWall>>(_obstacle);
}

bool FortressWall::IsSteelWall() const
{
	return std::holds_alternative<std::unique_ptr<SteelWall>>(_obstacle);
}

// NOTE: call when enemy team bonus pick up
void FortressWall::OnEnemyPickupShovel()
{
	_obstacle = std::unique_ptr<BrickWall>(nullptr);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const int>("ServerSend_FortressDied", _id);
	}
}

bool FortressWall::GetIsPassable() const
{
	bool result{true};

	std::visit([&result](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			result = uniqPtr.get()->GetIsPassable();
		}
	}, _obstacle);

	return result;
}

void FortressWall::SetIsPassable(const bool value)
{
	std::visit([value](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			uniqPtr.get()->SetIsPassable(value);
		}
	}, _obstacle);
}

bool FortressWall::GetIsDestructible() const
{
	bool result{false};

	std::visit([&result](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			result = uniqPtr.get()->GetIsDestructible();
		}
	}, _obstacle);

	return result;
}

void FortressWall::SetIsDestructible(const bool value)
{
	std::visit([value](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			uniqPtr.get()->SetIsDestructible(value);
		}
	}, _obstacle);
}

bool FortressWall::GetIsPenetrable() const
{
	bool result{true};

	std::visit([&result](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			result = uniqPtr.get()->GetIsPenetrable();
		}
	}, _obstacle);

	return result;
}

void FortressWall::SetIsPenetrable(const bool value)
{
	std::visit([value](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			uniqPtr.get()->SetIsPenetrable(value);
		}
	}, _obstacle);
}

int FortressWall::GetHealth() const
{
	int health{0};

	std::visit([&health](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			health = uniqPtr.get()->GetHealth();
		}
	}, _obstacle);

	return health;
}

void FortressWall::SetHealth(const int health)
{
	std::visit([health](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			uniqPtr.get()->SetHealth(health);
		}
	}, _obstacle);
}

ObjRectangle FortressWall::GetShape() const
{
	ObjRectangle shape{};

	std::visit([&shape](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			shape = uniqPtr.get()->GetShape();
		}
	}, _obstacle);

	return shape;
}

void FortressWall::SetShape(const ObjRectangle shape)
{
	std::visit([&shape](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			uniqPtr.get()->SetShape(shape);
		}
	}, _obstacle);
}

bool FortressWall::GetIsAlive() const
{
	return true;//NOTE: always true to preventing disposing broken FortressWalls
}

void FortressWall::SetIsAlive(const bool isAlive)
{
	std::visit([&isAlive](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			uniqPtr.get()->SetIsAlive(isAlive);
		}
	}, _obstacle);
}

std::string FortressWall::GetName() const
{
	std::string name;

	std::visit([&name](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			name = uniqPtr.get()->GetIsAlive();
		}
	}, _obstacle);

	return name;
}

int FortressWall::GetId() const
{
	int id{-1};

	std::visit([&id](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			id = uniqPtr.get()->GetIsAlive();
		}
	}, _obstacle);

	return id;
}
