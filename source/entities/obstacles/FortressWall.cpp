#include "entities/obstacles/FortressWall.h"
#include "components/EventSystem.h"
#include "entities/BaseObjProperty.h"
#include "entities/Pawns/Pawn.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/SteelWall.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"
#include "utils/ColliderUtils.h"
#include <algorithm>
#include <boost/uuid/uuid.hpp>
#include <string>

FortressWall::FortressWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events,
						   std::vector<std::shared_ptr<BaseObj>>* allObjects, const buuid uuid, const GameMode gameMode)
	: BaseObj{BaseObjProperty{.rect = rect, .health = 1, .uuid = uuid, .name = "FortressWall", .fraction = "Neutral"}}
	, _events{events}
	, _allObjects{allObjects}
	, _obstacle{std::make_unique<BrickWall>(rect, events, uuid, gameMode)}
	, _gameMode{gameMode}
{
	Subscribe();

	//disable replication for fortress _obstacle
	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_ObstacleSpawn", _rect, ObstacleType::Fortress, _uuid);
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

	_events->AddListener(
			"BonusShovel_StatusChange", _nameWithUuid,
			[this](const std::string& fraction, const bool isActive)
			{
				this->OnBonusShovel(fraction, isActive);
			});
}

void FortressWall::SubscribeAsClient()
{
	_events->AddListener(
			"ClientReceived_FortressChange", _nameWithUuid,
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
						this->OnShovelCooldownEnd();
					}
					else if (state == "ToSteel")
					{
						this->OnPlayerPickupShovel();
					}
				}
			});
}

void FortressWall::Unsubscribe() const { _events->RemoveAllListeners(_nameWithUuid); }

void FortressWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (std::holds_alternative<std::unique_ptr<BrickWall>>(_obstacle))
	{
		_events->EmitEvent("Statistics_BrickWallDied", author, fraction);
	}
	else
	{
		_events->EmitEvent("Statistics_SteelWallDied", author, fraction);
	}
}

// NOTE: call when enemy team pickup bonus
void FortressWall::OnEnemyPickupShovel()
{
	_obstacle = std::unique_ptr<BrickWall>(nullptr);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_FortressChange", "Died", _uuid);
	}
}

// NOTE: call when player team bonus pick up
void FortressWall::OnPlayerPickupShovel()
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [this](const std::shared_ptr<BaseObj>& object)
	{
		if (dynamic_cast<Pawn*>(object.get()))
		{
			return ColliderUtils::IsCollide(_rect, object->GetRect());
		}

		return false;
	});

	if (isFreeSpawnSpot)//Check if neared tank/bullet/bonus suppressed this spawn
	{
		_obstacle = std::make_unique<SteelWall>(_rect, _events, _uuid, _gameMode);

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_FortressChange", "ToSteel", _uuid);
		}
	}
}

void FortressWall::OnShovelCooldownEnd()
{
	if (GetHealth() > 0)
	{
		if (std::holds_alternative<std::unique_ptr<SteelWall>>(_obstacle))
		{
			_obstacle = std::make_unique<BrickWall>(_rect, _events, _uuid, _gameMode);
		}

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_FortressChange", "ToBrick", _uuid);
		}
	}
}

void FortressWall::OnBonusShovel(const std::string& fraction, const bool isActive)
{
	if (fraction == "EnemyTeam")
	{
		OnEnemyPickupShovel();
	}

	isActive ? OnPlayerPickupShovel() : OnShovelCooldownEnd();
}

void FortressWall::TakeDamage(const unsigned int damage, const std::string& damageAuthor, const std::string& damageFraction)
{
	const int health = std::visit([damage, &damageAuthor, &damageFraction](auto&& obstacle) -> int
	{
		if (obstacle)
		{
			obstacle->TakeDamage(damage, damageAuthor, damageFraction);
			return obstacle->GetHealth();
		}

		return -1;
	}, _obstacle);

	if (health <= 0)
	{
		_obstacle = std::unique_ptr<BrickWall>(nullptr);

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_FortressChange", "Died", _uuid);
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

bool FortressWall::GetIsPassable() const
{
	return std::visit([](auto&& obstacle)
	{
		return obstacle ? obstacle->GetIsPassable() : true;
	}, _obstacle);
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
	return std::visit([](auto&& obstacle)
	{
		return obstacle ? obstacle->GetIsDestructible() : false;
	}, _obstacle);
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
	return std::visit([](auto&& obstacle)
	{
		return obstacle ? obstacle->GetIsPenetrable() : true;
	}, _obstacle);
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
	return std::visit([](auto&& obstacle) -> int
	{
		return obstacle ? obstacle->GetHealth() : -1;
	}, _obstacle);
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
	return std::visit([](auto&& obstacle) -> ObjRectangle
	{
		return obstacle ? obstacle->GetRect() : ObjRectangle{};
	}, _obstacle);
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

//TODO: add to event system trait for auto conversion from std::string_view to string or working direct with view
std::string_view FortressWall::GetName() const
{
	return std::visit([this](auto&& obstacle) -> std::string_view
	{
		return obstacle ? obstacle->GetName() : "none";
	}, _obstacle);
}

using buuid = boost::uuids::uuid;

buuid FortressWall::GetUuid() const
{
	return std::visit([this](auto&& obstacle) -> buuid
	{
		return obstacle ? obstacle->GetUuid() : buuid{};
	}, _obstacle);
}
