#include "entities/obstacles/FortressWall.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/StatisticsEvents.h"
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
	: BaseObj{BaseObjProperty{.rect = rect, .health = 1, .uuid = uuid, .name = "FortressWall", .fraction = "Neutral"},
			  BrickWall::s_collision}//NOTE: FortressWall::GetIsPassable/Destructible/Penetrable fully delegate to
	//the held BrickWall/SteelWall variant below - this base value is never read,
	//BrickWall's tags used simply because FortressWall always starts as one
	, _events{events}
	, _allObjects{allObjects}
	, _obstacle{std::make_unique<BrickWall>(rect, events, uuid, gameMode)}
	, _gameMode{gameMode}
{
	Subscribe();

	//disable replication for fortress _obstacle
	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutObstacleSpawnEvent{.rect = _rect, .type = ObstacleType::Fortress, .uuid = _uuid});
	}
}

void FortressWall::Subscribe()
{
	if (_gameMode == GameMode::PlayAsClient)
	{
		SubscribeAsClient();
	}

	_subs.push_back(_events->AddListener(this, &FortressWall::OnBonusShovel));
}

void FortressWall::SubscribeAsClient()
{
	//NOTE: Client.cpp emits ClientInFortressChangeEvent here, split off the
	//ServerOutFortressChangeEvent the host side (below, PlayAsHost branches) uses for its own local
	//trigger - see ObstacleAndBonusEvents.h for why.
	_subs.push_back(_events->AddListener(Key(_uuid), this, &FortressWall::OnClientInFortressChange));
}

void FortressWall::OnClientInFortressChange(const ClientInFortressChangeEvent& event)
{
	if (event.state == "Died")
	{
		OnEnemyPickupShovel();
	}
	else if (event.state == "ToBrick")
	{
		OnShovelCooldownEnd();
	}
	else if (event.state == "ToSteel")
	{
		OnPlayerPickupShovel();
	}
}

void FortressWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (std::holds_alternative<std::unique_ptr<BrickWall>>(_obstacle))
	{
		_events->EmitEvent(BrickWallDiedEvent{.author = author, .fraction = fraction});
	}
	else
	{
		_events->EmitEvent(SteelWallDiedEvent{.author = author, .fraction = fraction});
	}
}

// NOTE: call when enemy team pickup bonus
void FortressWall::OnEnemyPickupShovel()
{
	_obstacle = std::unique_ptr<BrickWall>(nullptr);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutFortressChangeEvent{.state = "Died", .uuid = _uuid});
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
			_events->EmitEvent(ServerOutFortressChangeEvent{.state = "ToSteel", .uuid = _uuid});
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
			_events->EmitEvent(ServerOutFortressChangeEvent{.state = "ToBrick", .uuid = _uuid});
		}
	}
}

void FortressWall::OnBonusShovel(const BonusShovelStatusChangeEvent& event)
{
	if (event.fraction == "EnemyTeam")
	{
		OnEnemyPickupShovel();
	}

	event.isActive ? OnPlayerPickupShovel() : OnShovelCooldownEnd();
}

void FortressWall::TakeDamage(const unsigned int damage, const std::string& damageAuthor,
							  const std::string& damageFraction)
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
			_events->EmitEvent(ServerOutFortressChangeEvent{.state = "Died", .uuid = _uuid});
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

bool FortressWall::GetIsDestructible() const
{
	return std::visit([](auto&& obstacle)
	{
		return obstacle ? obstacle->GetIsDestructible() : false;
	}, _obstacle);
}

bool FortressWall::GetIsPenetrable() const
{
	return std::visit([](auto&& obstacle)
	{
		return obstacle ? obstacle->GetIsPenetrable() : true;
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
	return true;//NOTE: always true to preventing disposing of broken FortressWalls
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

std::string FortressWall::GetName() const
{
	return std::visit([](auto&& obstacle) -> std::string
	{
		return obstacle ? obstacle->GetName() : std::string{};
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
