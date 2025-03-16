#include "../../headers/pawns/CoopBot.h"
#include "../../headers/EventSystem.h"
#include "../../headers/LineOfSight.h"
#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>

CoopBot::CoopBot(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
                 const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                 std::shared_ptr<EventSystem> events, std::string name, std::string fraction,
                 std::shared_ptr<BulletPool> bulletPool, const GameMode gameMode, const int id, const int tier)
	: Bot{rect,
	      color,
	      health,
	      std::move(window),
	      direction,
	      speed,
	      allObjects,
	      std::move(events),
	      std::move(name),
	      std::move(fraction),
	      std::move(bulletPool),
	      gameMode,
	      id,
	      tier} {}

CoopBot::~CoopBot() = default;

void CoopBot::HandleLineOfSight(const Direction dir)
{
	const FPoint bulletSize = {.x = GetBulletWidth(), .y = GetBulletHeight()};
	LineOfSight lineOfSight(_shape, _window->size, bulletSize, _allObjects, this);

	const auto& upSideObstacles = lineOfSight.GetUpSideObstacles();
	if (HandleSideObstacles(UP, upSideObstacles))
	{
		return;
	}

	const auto& leftSideObstacles = lineOfSight.GetLeftSideObstacles();
	if (HandleSideObstacles(LEFT, leftSideObstacles))
	{
		return;
	}

	const auto& downSideObstacles = lineOfSight.GetDownSideObstacles();
	if (HandleSideObstacles(DOWN, downSideObstacles))
	{
		return;
	}

	const auto& rightSideObstacles = lineOfSight.GetRightSideObstacles();
	if (HandleSideObstacles(RIGHT, rightSideObstacles))
	{
		return;
	}

	// fire on obstacle if player not found
	float shootDistance{0.f};
	float bulletOffset{0.f};
	std::shared_ptr<BaseObj> nearestObstacle{nullptr};
	if (dir == UP && !upSideObstacles.empty())
	{
		if (nearestObstacle = upSideObstacles.front().lock();
			nearestObstacle)
		{
			shootDistance = GetY() - nearestObstacle->GetY();
			bulletOffset = bulletSize.y;
		}
	}

	if (dir == LEFT && !leftSideObstacles.empty())
	{
		if (nearestObstacle = leftSideObstacles.front().lock();
			nearestObstacle)
		{
			shootDistance = GetX() - nearestObstacle->GetX();
			bulletOffset = bulletSize.x;
		}
	}

	if (dir == DOWN && !downSideObstacles.empty())
	{
		if (nearestObstacle = downSideObstacles.front().lock();
			nearestObstacle)
		{
			shootDistance = nearestObstacle->GetY() - GetY();
			bulletOffset = bulletSize.y;
		}
	}

	if (dir == RIGHT && !rightSideObstacles.empty())
	{
		if (nearestObstacle = rightSideObstacles.front().lock();
			nearestObstacle)
		{
			shootDistance = nearestObstacle->GetX() - GetX();
			bulletOffset = bulletSize.x;
		}
	}

	if (nearestObstacle
	    && (nearestObstacle->GetIsDestructible() || _tier > 2)
	    && !dynamic_cast<WaterTile*>(nearestObstacle.get())
	    // && !dynamic_cast<BushesTile*>(nearestObstacle.get())
	    // && !dynamic_cast<IceTile*>(nearestObstacle.get())
	    && !dynamic_cast<FortressWall*>(nearestObstacle.get())
	    && !IsAlly(nearestObstacle))
	{
		if (shootDistance > _bulletDamageRadius + bulletOffset)
		{
			Shot();
		}
	}
}

void CoopBot::TickUpdate(const float deltaTime)
{
	Bot::TickUpdate(deltaTime);

	// shot
	if (TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		HandleLineOfSight(GetDirection());
	}
}

void CoopBot::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("CoopBotHit", author, fraction);

	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("CoopBotDied", author, fraction);
	}
}
