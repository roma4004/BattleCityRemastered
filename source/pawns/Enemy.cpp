#include "../../headers/pawns/Enemy.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/components/LineOfSight.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>

//TODO: if enemy see bullets they should tru or prioritize move aside
Enemy::Enemy(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
             const Direction dir, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
             const std::shared_ptr<EventSystem>& events, std::string name, std::string fraction,
             std::shared_ptr<BulletPool> bulletPool, const GameMode gameMode, const int id, const int tier)
	: Bot{rect,
	      color,
	      health,
	      std::move(window),
	      dir,
	      speed,
	      allObjects,
	      std::move(events),
	      std::move(name),
	      std::move(fraction),
	      std::move(bulletPool),
	      gameMode,
	      id,
	      tier} {}

Enemy::~Enemy() = default;

void Enemy::TickUpdate(const float deltaTime)
{
	if (!this->_timer.isActive)
	{
		this->Bot::TickUpdate(deltaTime);
	}

	// shot
	if (TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		HandleLineOfSight(GetDirection());

		if (_nearestSeenObstacle
		    && (_nearestSeenObstacle->GetIsDestructible() || _tier > 2)
		    && !dynamic_cast<WaterTile*>(_nearestSeenObstacle.get())
		    // && !dynamic_cast<BushesTile*>(_nearestSeenObstacle.get())
		    // && !dynamic_cast<IceTile*>(_nearestSeenObstacle.get())
		    && !dynamic_cast<FortressWall*>(_nearestSeenObstacle.get())
		    && !IsAlly(_nearestSeenObstacle))
		{
			if (_shootDistance > _bulletDamageRadius + _bulletOffset)//TODO: cover this by test
			{
				Shot();
			}
		}
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const Direction>(
				"ServerSend_Pos", "Enemy" + std::to_string(GetId()), GetPos(), GetDirection());
	}
}
