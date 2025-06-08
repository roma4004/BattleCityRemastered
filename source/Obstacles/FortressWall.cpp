#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/Pawns/Pawn.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <string>
#include <boost/uuid/uuid.hpp>

FortressWall::FortressWall(ObjRectangle rect, std::shared_ptr<Window> window,
                           const std::shared_ptr<EventSystem>& events,
                           std::vector<std::shared_ptr<BaseObj>>* allObjects, const buuid uuid, const GameMode gameMode,
                           std::shared_ptr<IDrawable> textureManager)
	: BaseObj{rect, 0x924b00, 1, uuid, "FortressWall", "Neutral"},
	  _gameMode{gameMode},
	  _window{window},
	  _events{events},
	  _textureManager{textureManager},
	  _allObjects{allObjects},
	  _obstacle{std::make_unique<BrickWall>(rect, window, events, uuid, gameMode, textureManager)}
{
	//TODO: fix fortress replication
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
	_events->AddListener<const std::string&, const buuid&>(
			"ClientReceived_FortressChange", _name,
			[this](const std::string& state, const buuid& uuid)
			{
				if (uuid == _uuid)
				{
					if (state == "Died")
					{
						this->OnEnemyPickupShovel();
					}
					else if (state == "ToSteel")
					{
						this->OnPlayerPickupShovel();
					}
					else if (state == "ToBrick")
					{
						this->OnPlayerShovelCooldownEnd();
					}
				}
			});
}

void FortressWall::SubscribeBonus()
{
	_events->AddListener<const std::string&, const std::string&, const std::chrono::milliseconds>(
			//TODO: remove duration for bonuses
			"BonusShovel", _name,
			[this](const std::string& /*author*/, const std::string& fraction, const std::chrono::milliseconds duration)
			{
				this->OnBonusShovelPickup(fraction, duration);
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
	_events->RemoveListener<const std::string&, const buuid&>("ClientReceived_FortressChange", _name);
}

void FortressWall::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const std::string&, const std::chrono::milliseconds>(
			"BonusShovel", _name);
}

void FortressWall::Draw(const BaseObj* /*obj*/) const {}

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
		_events->EmitEvent<const std::string&, const std::string&>("Statistics_BrickWallDied", author, fraction);
	}
	else
	{
		_events->EmitEvent<const std::string&, const std::string&>("Statistics_SteelWallDied", author, fraction);
	}
}

void FortressWall::OnPlayerShovelCooldownEnd()
{
	if (GetHealth() > 0)
	{
		if (std::holds_alternative<std::unique_ptr<SteelWall>>(_obstacle))
		{
			_obstacle = std::make_unique<BrickWall>(_rect, _window, _events, _uuid, _gameMode, _textureManager);
		}

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const buuid&>(
					"ServerSend_FortressChange", "ToBrick", _uuid);
		}
	}
}

//TODO: should be private and friend bonusShovel to activate
// NOTE: call when player team bonus pick up
void FortressWall::OnPlayerPickupShovel()
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [this](const std::shared_ptr<BaseObj>& object)
	{
		if (object.get() == nullptr)
		{
			return false;
		}

		if (dynamic_cast<Pawn*>(object.get()))
		{
			return ColliderUtils::IsCollide(_rect, object->GetRect());
		}

		return false;
	});

	if (isFreeSpawnSpot)//Check if neared tank/bullet/bonus suppressed this spawn
	{
		_obstacle = std::make_unique<SteelWall>(_rect, _window, _events, _uuid, _gameMode, _textureManager);

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const buuid&>(
					"ServerSend_FortressChange", "ToSteel", _uuid);
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
			_events->EmitEvent<const std::string&, const buuid&>("ServerSend_FortressChange", "Died", _uuid);
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

void FortressWall::OnBonusShovelPickup(const std::string& fraction, const std::chrono::milliseconds duration)
{
	if (fraction == "PlayerTeam")
	{
		if (this->_shovel.isActive)
		{
			this->_shovel.cooldown += duration;

			return;
		}

		this->_shovel.isActive = true;
		this->_shovel.cooldown = duration;
		this->OnPlayerPickupShovel();
	}
	else if (fraction == "EnemyTeam")
	{
		this->_shovel.isActive = false;
		this->OnEnemyPickupShovel();
	}

	this->_shovel.activateTime = std::chrono::system_clock::now();
}

// NOTE: call when enemy team bonus pick up
void FortressWall::OnEnemyPickupShovel()
{
	_obstacle = std::unique_ptr<BrickWall>(nullptr);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const buuid&>("ServerSend_FortressChange", "Died", _uuid);
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

ObjRectangle FortressWall::GetRect() const
{
	ObjRectangle rect{};

	std::visit([&rect](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			rect = uniqPtr.get()->GetRect();
		}
	}, _obstacle);

	return rect;
}

void FortressWall::SetRect(const ObjRectangle rect)
{
	std::visit([&rect](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			uniqPtr.get()->SetRect(rect);
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
			name = uniqPtr.get()->GetName();
		}
	}, _obstacle);

	return name;
}

using buuid = boost::uuids::uuid;

buuid FortressWall::GetUuid() const
{
	buuid uuid{};

	std::visit([&uuid](auto&& uniqPtr)
	{
		if (uniqPtr)
		{
			uuid = uniqPtr.get()->GetUuid();
		}
	}, _obstacle);

	return uuid;
}
