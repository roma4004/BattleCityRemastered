#include "entities/obstacles/FortressWall.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/StatisticsEvents.h"
#include "entities/BaseObjProperty.h"
#include "entities/pawns/Pawn.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/SteelWall.h"
#include "enums/FortressState.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"
#include "utils/ColliderUtils.h"
#include "utils/Uuid.h"
#include <algorithm>
#include <string>

FortressWall::FortressWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events,
						   std::vector<std::shared_ptr<BaseObj>>* allObjects, const Uuid uuid, const GameMode gameMode)
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
	if (IsHost(_gameMode))
	{
		_events->EmitEvent(ObstacleSpawnedEvent{.pos = GetPos(), .type = ObstacleType::Fortress, .uuid = _uuid});
	}
}

void FortressWall::Subscribe()
{
	if (IsClient(_gameMode))
	{
		SubscribeAsClient();
	}

	_subs.push_back(_events->AddListener(this, &FortressWall::OnBonusShovel));
}

void FortressWall::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_uuid), this, &FortressWall::OnFortressChanged));
}

void FortressWall::OnFortressChanged(const FortressChangedEvent& event)
{
	switch (event.state)
	{
		case FortressState::Died:
			OnEnemyPickupShovel();
			break;
		case FortressState::ToBrick:
			OnShovelCooldownEnd();
			break;
		case FortressState::ToSteel:
			OnPlayerPickupShovel();
			break;
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

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(FortressChangedEvent{.state = FortressState::Died, .uuid = _uuid});
	}
}

// NOTE: call when player team pickup bonus
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

		if (IsHost(_gameMode))
		{
			_events->EmitEvent(FortressChangedEvent{.state = FortressState::ToSteel, .uuid = _uuid});
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

		if (IsHost(_gameMode))
		{
			_events->EmitEvent(FortressChangedEvent{.state = FortressState::ToBrick, .uuid = _uuid});
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

void FortressWall::TakeDamage(const unsigned int damage, const std::string& author, const std::string& fraction)
{
	const int health = std::visit([damage, &author, &fraction](auto&& obstacle) -> int
	{
		if (obstacle)
		{
			obstacle->TakeDamage(damage, author, fraction);
			return obstacle->GetHealth();
		}

		return -1;
	}, _obstacle);

	if (health <= 0)
	{
		_obstacle = std::unique_ptr<BrickWall>(nullptr);

		if (IsHost(_gameMode))
		{
			_events->EmitEvent(FortressChangedEvent{.state = FortressState::Died, .uuid = _uuid});
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

Uuid FortressWall::GetUuid() const
{
	return std::visit([this](auto&& obstacle) -> Uuid
	{
		return obstacle ? obstacle->GetUuid() : Uuid{};
	}, _obstacle);
}
