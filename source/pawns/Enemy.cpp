#include "../../headers/pawns/Enemy.h"
#include "../../headers/EventSystem.h"
#include "../../headers/LineOfSight.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>

//TODO: if enemy see bullets they should tru or prioritize move aside
Enemy::Enemy(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
             const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
             const std::shared_ptr<EventSystem>& events, std::string name, std::string fraction,
             std::shared_ptr<BulletPool> bulletPool, const GameMode gameMode, const int id)
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
	      id} {}

Enemy::~Enemy() = default;

void Enemy::HandleLineOfSight(const Direction dir)
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

	// TODO: write logic if seen bullet flying toward(head-on) to this tank, need shoot to intercept
	// if (isBullet && isOpposite(bullet->GetDirection))
	// {
	// 	Shot();
	// }

	// fire on obstacle if player not found
	float shootDistance{0.f};
	float bulletOffset{0.f};
	std::shared_ptr<BaseObj> nearestObstacle{nullptr};
	if (dir == UP && !upSideObstacles.empty())
	{
		nearestObstacle = upSideObstacles.front().lock();
		if (nearestObstacle)
		{
			shootDistance = GetY() - nearestObstacle->GetY();
			bulletOffset = GetBulletHeight();
		}
	}

	if (dir == LEFT && !leftSideObstacles.empty())
	{
		nearestObstacle = leftSideObstacles.front().lock();
		if (nearestObstacle)
		{
			shootDistance = GetX() - nearestObstacle->GetX();
			bulletOffset = GetBulletWidth();
		}
	}

	if (dir == DOWN && !downSideObstacles.empty())
	{
		nearestObstacle = downSideObstacles.front().lock();
		if (nearestObstacle)
		{
			shootDistance = nearestObstacle->GetY() - GetY();
			bulletOffset = GetBulletHeight();
		}
	}

	if (dir == RIGHT && !rightSideObstacles.empty())
	{
		nearestObstacle = rightSideObstacles.front().lock();
		if (nearestObstacle)
		{
			shootDistance = nearestObstacle->GetX() - GetX();
			bulletOffset = GetBulletWidth();
		}
	}

	if (nearestObstacle && nearestObstacle->GetIsDestructible()
		&& !IsAlly(nearestObstacle))
	{
		if (shootDistance > _bulletDamageRadius + bulletOffset)//TODO: cover this by test
		{
			Shot();
		}
	}
}

void Enemy::TickUpdate(const float deltaTime)
{
	Bot::TickUpdate(deltaTime);

	// shot
	if (TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		HandleLineOfSight(GetDirection());
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const Direction>(
				"ServerSend_Pos", "Enemy" + std::to_string(GetId()), GetPos(), GetDirection());
	}
}

void Enemy::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("EnemyHit", author, fraction);

	if (GetHealth() < 1)
	{
		//TODO: move to event from statistic when last tank died
		_events->EmitEvent<const std::string&, const std::string&>("EnemyDied", author, fraction);
	}
}

void Enemy::TakeDamage(const int damage)
{
	Tank::TakeDamage(damage);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string, const int>("ServerSend_Health", GetName(), GetHealth());
	}
}
