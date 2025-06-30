#include "entities/obstacles/FortressWall.h"
#include "components/EventSystem.h"
#include "entities/Pawns/Pawn.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/SteelWall.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"
#include "utils/ColliderUtils.h"
#include <algorithm>
#include <string>
#include <boost/uuid/uuid.hpp>

FortressWall::FortressWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events,
                           std::vector<std::shared_ptr<BaseObj>>* allObjects, const buuid uuid, const GameMode gameMode)
	: BaseObj{rect, 0x924b00, 1, uuid, "FortressWall", "Neutral"},
	  _events{events},
	  _allObjects{allObjects},
	  _obstacle{std::make_unique<BrickWall>(rect, events, uuid, gameMode, false)},
	  _gameMode{gameMode}
{
	Subscribe();

	//disable replication for fortress _obstacle
	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent<const ObjRectangle, const ObstacleType, const buuid&>(
				"ServerSend_ObstacleSpawn", _rect, ObstacleType::Fortress, uuid);
	}
}

FortressWall::~FortressWall()
{
	Unsubscribe();
}

void FortressWall::Subscribe()
{
	if (_gameMode == GameMode::PlayAsClient)
	{
		SubscribeAsClient();
	}

	SubscribeBonus();
}

void FortressWall::SubscribeAsClient()
{
	_events->AddListener<const std::string&, const buuid&>(
			"ClientReceived_FortressChange", _nameWithUuid,//TODO: maybe register with name of brick
			[this](const std::string& state, const buuid& uuid)
			{
				if (uuid == _uuid)
				{
					if (state == "Died")
					{
						this->OnEnemyPickupShovel();
					}
					else if (state == "ToBrick")
					{
						this->OnPlayerShovelCooldownEnd();
					}
					else if (state == "ToSteel")
					{
						this->OnPlayerPickupShovel();
					}
				}
			});
}

void FortressWall::SubscribeBonus()
{
	_events->AddListener("BonusShovelOnPlayerPickup", _nameWithUuid, [this]() { this->OnPlayerPickupShovel(); });
	_events->AddListener("BonusShovelOnCooldownEnd", _nameWithUuid, [this]() { this->OnPlayerShovelCooldownEnd(); });
	_events->AddListener("BonusShovelOnEnemyPickup", _nameWithUuid, [this]() { this->OnEnemyPickupShovel(); });
}

void FortressWall::Unsubscribe() const
{
	if (_gameMode == GameMode::PlayAsClient)
	{
		UnsubscribeAsClient();
	}

	UnsubscribeBonus();
}

void FortressWall::UnsubscribeAsClient() const
{
	_events->RemoveListener<const std::string&, const buuid&>("ClientReceived_FortressChange", _nameWithUuid);
}

void FortressWall::UnsubscribeBonus() const
{
	_events->RemoveListener("BonusShovelOnPlayerPickup", _nameWithUuid);
	_events->RemoveListener("BonusShovelOnCooldownEnd", _nameWithUuid);
	_events->RemoveListener("BonusShovelOnEnemyPickup", _nameWithUuid);
}

void FortressWall::Draw(const BaseObj* /*obj*/) const {}

void FortressWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (std::holds_alternative<std::unique_ptr<BrickWall>>(_obstacle))
	{
		_events->EmitEvent<const std::string&, const std::string&>("Statistics_BrickWallDied", author, fraction);
		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const int, const buuid&>(
					"ServerSend_Health", GetName(), GetHealth(), GetUuid());
		}
	}
	else
	{
		_events->EmitEvent<const std::string&, const std::string&>("Statistics_SteelWallDied", author, fraction);
		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const int, const buuid&>(
					"ServerSend_Health", GetName(), GetHealth(), GetUuid());
		}
	}
}

void FortressWall::OnPlayerShovelCooldownEnd()
{
	if (GetHealth() > 0)
	{
		if (std::holds_alternative<std::unique_ptr<SteelWall>>(_obstacle))
		{
			_obstacle = std::make_unique<BrickWall>(_rect, _events, _uuid, _gameMode, false);
		}

		if (_gameMode == GameMode::PlayAsHost)
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
		if (object == nullptr)
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
		_obstacle = std::make_unique<SteelWall>(_rect, _events, _uuid, _gameMode, false);

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const buuid&>(
					"ServerSend_FortressChange", "ToSteel", _uuid);
		}
	}
}

void FortressWall::TakeDamage(const int damage)
{
	int health{0};

	std::visit([damage, &health](auto&& obstacle)
	{
		if (obstacle)
		{
			obstacle->TakeDamage(damage);
			health = obstacle->GetHealth();
		}
	}, _obstacle);

	if (health <= 0)
	{
		_obstacle = std::unique_ptr<BrickWall>(nullptr);

		if (_gameMode == GameMode::PlayAsHost)
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

// NOTE: call when enemy team pickup bonus
void FortressWall::OnEnemyPickupShovel()
{
	_obstacle = std::unique_ptr<BrickWall>(nullptr);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const buuid&>("ServerSend_FortressChange", "Died", _uuid);
	}
}

bool FortressWall::GetIsPassable() const
{
	bool result{true};

	std::visit([&result](auto&& obstacle)
	{
		if (obstacle)
		{
			result = obstacle->GetIsPassable();
		}
	}, _obstacle);

	return result;
}

void FortressWall::SetIsPassable(const bool value)
{
	std::visit([value](auto&& obstacle)
	{
		if (obstacle)
		{
			obstacle->SetIsPassable(value);
		}
	}, _obstacle);
}

bool FortressWall::GetIsDestructible() const
{
	bool result{false};

	std::visit([&result](auto&& obstacle)
	{
		if (obstacle)
		{
			result = obstacle->GetIsDestructible();
		}
	}, _obstacle);

	return result;
}

void FortressWall::SetIsDestructible(const bool value)
{
	std::visit([value](auto&& obstacle)
	{
		if (obstacle)
		{
			obstacle->SetIsDestructible(value);
		}
	}, _obstacle);
}

bool FortressWall::GetIsPenetrable() const
{
	bool result{true};

	std::visit([&result](auto&& obstacle)
	{
		if (obstacle)
		{
			result = obstacle->GetIsPenetrable();
		}
	}, _obstacle);

	return result;
}

void FortressWall::SetIsPenetrable(const bool value)
{
	std::visit([value](auto&& obstacle)
	{
		if (obstacle)
		{
			obstacle->SetIsPenetrable(value);
		}
	}, _obstacle);
}

int FortressWall::GetHealth() const
{
	int health{0};

	std::visit([&health](auto&& obstacle)
	{
		if (obstacle)
		{
			health = obstacle->GetHealth();
		}
	}, _obstacle);

	return health;
}

void FortressWall::SetHealth(const int health)
{
	std::visit([health](auto&& obstacle)
	{
		if (obstacle)
		{
			obstacle->SetHealth(health);
		}
	}, _obstacle);
}

ObjRectangle FortressWall::GetRect() const
{
	ObjRectangle rect{};

	std::visit([&rect](auto&& obstacle)
	{
		if (obstacle)
		{
			rect = obstacle->GetRect();
		}
	}, _obstacle);

	return rect;
}

void FortressWall::SetRect(const ObjRectangle rect)
{
	std::visit([&rect](auto&& obstacle)
	{
		if (obstacle)
		{
			obstacle->SetRect(rect);
		}
	}, _obstacle);
}

bool FortressWall::GetIsAlive() const
{
	return true;//NOTE: always true to preventing disposing broken FortressWalls
}

void FortressWall::SetIsAlive(const bool isAlive)
{
	std::visit([&isAlive](auto&& obstacle)
	{
		if (obstacle)
		{
			obstacle->SetIsAlive(isAlive);
		}
	}, _obstacle);
}

const std::string& FortressWall::GetName() const
{
	return std::visit([&](auto&& obstacle)
	{
		if (obstacle)
		{
			return obstacle->GetName();
		}

		return _name;//TODO: use string_view instead
	}, _obstacle);
}

using buuid = boost::uuids::uuid;

buuid FortressWall::GetUuid() const
{
	buuid uuid{};

	std::visit([&uuid](auto&& obstacle)
	{
		if (obstacle)
		{
			uuid = obstacle->GetUuid();
		}
	}, _obstacle);

	return uuid;
}
