#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/EventSystem.h"
#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <string>

FortressWall::FortressWall(const ObjRectangle& rect, std::shared_ptr<Window> window,
                           const std::shared_ptr<EventSystem>& events,
                           std::vector<std::shared_ptr<BaseObj>>* allObjects, const int id)
	: BaseObj{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1}, 0x924b00, 1},
	  _rect{rect},
	  _window{window},
	  _events{events},
	  _allObjects{allObjects},
	  _name{"fortressWall" + std::to_string(id)},//TODO: change name for statistic
	  _obstacle{std::make_unique<BrickWall>(rect, window, events, id)},
	  _id{id}
{
	Subscribe();
}

FortressWall::~FortressWall()
{
	Unsubscribe();
}

void FortressWall::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_events->AddListener<const int>("ClientReceived_FortressDied", _name, [this](const int id)
	{
		if (id != _id)
		{
			return;
		}

		this->Hide();
	});//TODO: enable only in client mode
	//TODO: replicate bonusShovel, to replace with steel
//TODO: split subscription subBonus, subAsClient etc.
	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusShovel", _name,
			[this](const std::string& /*author*/, const std::string& fraction, const int bonusDurationTimeMs)
			{
				if (fraction == "PlayerTeam")
				{
					if (this->_isActiveShovel)
					{
						this->_cooldownShovelMs += bonusDurationTimeMs;

						return;
					}

					this->_isActiveShovel = true;
					this->BonusShovelSwitch();
					this->_cooldownShovelMs = bonusDurationTimeMs;
				}
				else if (fraction == "EnemyTeam")
				{
					this->_isActiveShovel = false;
					this->Hide();
				}

				this->_activateTimeShovel = std::chrono::system_clock::now();
			});
}

void FortressWall::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	// _events->RemoveListener<const float>("ClientReceived_BrickWall" + std::to_string(_id) + "Health", _name);
	// _events->RemoveListener<const float>("ClientReceived_SteelWall"  + std::to_string(_obstacleId) + "Health", _name);
	_events->RemoveListener<const float>("TickUpdate", _name);
	_events->RemoveListener("Draw", _name);
	_events->RemoveListener<const std::string&, const std::string&, int>("BonusShovel", _name);
}

void FortressWall::Draw() const {}

void FortressWall::TickUpdate(const float /*deltaTime*/)
{
	// bonus disable timer
	if (_isActiveShovel && TimeUtils::IsCooldownFinish(_activateTimeShovel, _cooldownShovelMs))
	{
		_isActiveShovel = false;
		_cooldownShovelMs = 0;
		BonusShovelSwitch();
	}
}

//TODO: check maybe delete this method
void FortressWall::SendDamageStatistics(const std::string& /*author*/, const std::string& /*fraction*/) {}

//TODO: should be private and friend bonusShovel to activate
// NOTE: call when player team bonus pick up
void FortressWall::BonusShovelSwitch()
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [this](const std::shared_ptr<BaseObj>& object)
	{
		if (dynamic_cast<FortressWall*>(object.get()))
		{
			return false;
		}

		return ColliderUtils::IsCollide(_rect, object->GetShape());
	});

	if (isFreeSpawnSpot)//Check if neared tank/bullet/bonus suppressed this spawn
	{
		if (std::holds_alternative<std::unique_ptr<BrickWall>>(_obstacle))
		{
			_obstacle = std::make_unique<SteelWall>(_rect, _window, _events, _id);
			//TODO: send event for replication thar brick need to appear at client side
		}
		else
		{
			_obstacle = std::make_unique<BrickWall>(_rect, _window, _events, _id);
			//TODO: send event for replication thar steel need to appear at client side
		}
	}
}

//TODO: write more unit test for fortressWall bonus logic work
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
		_events->EmitEvent<const int>("ServerSend_FortressDied", _id);//TODO: disable in client mode
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
void FortressWall::Hide()
{
	if (std::holds_alternative<std::unique_ptr<BrickWall>>(_obstacle))
	{
		_obstacle = std::unique_ptr<BrickWall>(nullptr);
		_events->EmitEvent<const int>("ServerSend_FortressDied", _id);//TODO: disable in client mode
	}
	else
	{
		_obstacle = std::unique_ptr<SteelWall>(nullptr);
		_events->EmitEvent<const int>("ServerSend_FortressDied", _id);//TODO: disable in client mode
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
	bool result{true};

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
	bool isAlive{false};

	std::visit([&isAlive](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			isAlive = uniqPtr.get()->GetIsAlive();
		}
	}, _obstacle);

	return isAlive;
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
