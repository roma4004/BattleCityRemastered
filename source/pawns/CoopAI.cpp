#include "../../headers/pawns/CoopAI.h"
#include "../../headers/EventSystem.h"
#include "../../headers/LineOfSight.h"
#include "../../headers/behavior/MoveLikeAIBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/interfaces/IPickupableBonus.h"
#include "../../headers/pawns/Enemy.h"
#include "../../headers/pawns/PlayerOne.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <chrono>

CoopAI::CoopAI(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
               const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
               std::shared_ptr<EventSystem> events, std::string name, std::string fraction,
               std::shared_ptr<BulletPool> bulletPool, const GameMode gameMode, const int id)
	: Tank{rect,
	       color,
	       health,
	       std::move(window),
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::make_unique<MoveLikeAIBeh>(this, allObjects),
	       std::make_shared<ShootingBeh>(this, allObjects, events, std::move(bulletPool)),
	       std::move(name),
	       std::move(fraction),
	       gameMode,
	       id},
	  _distDirection(0, 3), _distTurnRate(1000/*ms*/, 5000/*ms*/)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	std::random_device rd;
	_gen = std::mt19937(static_cast<unsigned int>(
		std::chrono::high_resolution_clock::now().time_since_epoch().count() + rd()));

	Subscribe();

	events->EmitEvent(_name + "_Spawn");
}

CoopAI::~CoopAI()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");
}

void CoopAI::Subscribe()
{
	SubscribeAsHost();

	SubscribeBonus();
}

void CoopAI::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		// bonuses
		if (this->_isActiveTimer && TimeUtils::IsCooldownFinish(this->_activateTimeTimer, this->_cooldownTimer))
		{
			this->_isActiveTimer = false;
			this->_cooldownTimer = 0;
		}

		if (_isActiveHelmet && TimeUtils::IsCooldownFinish(this->_activateTimeHelmet, this->_cooldownHelmet))
		{
			this->_isActiveHelmet = false;
			this->_cooldownHelmet = 0;
		}

		if (!this->_isActiveTimer)
		{
			this->TickUpdate(deltaTime);
		}
	});
}

void CoopAI::SubscribeBonus()
{
	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusTimer", _name,
			[this](const std::string& /*author*/, const std::string& fraction, const int bonusDurationTimeMs)
			{
				if (fraction != this->_fraction)
				{
					this->_isActiveTimer = true;
					this->_cooldownTimer += bonusDurationTimeMs;
					this->_activateTimeTimer = std::chrono::system_clock::now();
				}
			});

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusHelmet", _name,
			[this](const std::string& author, const std::string& fraction, const int bonusDurationTimeMs)
			{
				if (fraction == this->_fraction && author == this->_name)
				{
					this->_isActiveHelmet = true;
					this->_cooldownHelmet += bonusDurationTimeMs;
					this->_activateTimeHelmet = std::chrono::system_clock::now();
				}
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusGrenade", _name,
			[this](const std::string& /*author*/, const std::string& fraction)
			{
				if (fraction != this->_fraction)
				{
					this->TakeDamage(GetHealth());
				}
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusStar", _name,
			[this](const std::string& author, const std::string& fraction)
			{
				if (fraction == this->_fraction && author == this->_name)
				{
					this->SetHealth(this->GetHealth() + 50);
					if (this->_tier > 4)
					{
						return;
					}

					++this->_tier;

					this->SetSpeed(this->GetSpeed() * 1.10f);
					this->SetBulletSpeed(this->GetBulletSpeed() * 1.10f);
					this->SetBulletDamage(this->GetBulletDamage() + 15);
					this->SetFireCooldownMs(this->GetFireCooldownMs() - 150);
					this->SetBulletDamageRadius(this->GetBulletDamageRadius() * 1.25f);
				}
			});
}

void CoopAI::Unsubscribe() const
{
	UnsubscribeAsHost();

	UnsubscribeBonus();
}

void CoopAI::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _name);
}

void CoopAI::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const std::string&, int>("BonusTimer", _name);
	_events->RemoveListener<const std::string&, const std::string&, int>("BonusHelmet", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusGrenade", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusStar", _name);
}

bool CoopAI::IsEnemy(const std::weak_ptr<BaseObj>& obstacle)
{
	if (const std::shared_ptr<BaseObj> isEnemySeen = obstacle.lock(); dynamic_cast<Enemy*>(isEnemySeen.get()))
	{
		return true;
	}

	return false;
}

bool CoopAI::IsBonus(const std::weak_ptr<BaseObj>& obstacle)
{
	if (const std::shared_ptr<BaseObj> isBonusSeen = obstacle.lock();
		dynamic_cast<IPickupableBonus*>(isBonusSeen.get()))
	{
		return true;
	}

	return false;
}

void CoopAI::HandleLineOfSight(const Direction dir)
{
	const FPoint bulletSize = {.x = GetBulletWidth(), .y = GetBulletHeight()};
	const FPoint bulletHalf = {.x = bulletSize.x / 2.f, .y = bulletSize.y / 2.f};
	LineOfSight lineOfSight(_shape, _window->size, bulletHalf, _allObjects, this);

	const auto upSideObstacles = lineOfSight.GetUpSideObstacles();
	if (!upSideObstacles.empty())
	{
		if (IsEnemy(upSideObstacles.front()))
		{
			SetDirection(UP);
			Shot();

			return;
		}

		if (IsBonus(upSideObstacles.front()))
		{
			SetDirection(UP);

			return;
		}
	}

	const auto leftSideObstacles = lineOfSight.GetLeftSideObstacles();
	if (!leftSideObstacles.empty())
	{
		if (IsEnemy(leftSideObstacles.front()))
		{
			SetDirection(LEFT);
			Shot();

			return;
		}

		if (IsBonus(leftSideObstacles.front()))
		{
			SetDirection(LEFT);

			return;
		}
	}

	const auto downSideObstacles = lineOfSight.GetDownSideObstacles();
	if (!downSideObstacles.empty())
	{
		if (IsEnemy(downSideObstacles.front()))
		{
			SetDirection(DOWN);
			Shot();

			return;
		}

		if (IsBonus(downSideObstacles.front()))
		{
			SetDirection(DOWN);

			return;
		}
	}

	const auto rightSideObstacles = lineOfSight.GetRightSideObstacles();
	if (!rightSideObstacles.empty())
	{
		if (IsEnemy(rightSideObstacles.front()))
		{
			SetDirection(RIGHT);
			Shot();

			return;
		}

		if (IsBonus(rightSideObstacles.front()))
		{
			SetDirection(RIGHT);

			return;
		}
	}

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
			bulletOffset = bulletSize.y;
		}
	}
	
	if (dir == LEFT && !leftSideObstacles.empty())
	{
		nearestObstacle = leftSideObstacles.front().lock();
		if (nearestObstacle)
		{
			shootDistance = GetX() - nearestObstacle->GetX();
			bulletOffset = bulletSize.x;
		}
	}
	
	if (dir == DOWN && !downSideObstacles.empty())
	{
		nearestObstacle = downSideObstacles.front().lock();
		if (nearestObstacle)
		{
			shootDistance = nearestObstacle->GetY() - GetY();
			bulletOffset = bulletSize.y;
		}
	}
	
	if (dir == RIGHT && !rightSideObstacles.empty())
	{
		nearestObstacle = rightSideObstacles.front().lock();
		if (nearestObstacle)
		{
			shootDistance = nearestObstacle->GetX() - GetX();
			bulletOffset = bulletSize.x;
		}
	}

	if (nearestObstacle
	    && nearestObstacle.get()
	    && nearestObstacle->GetIsDestructible()
	    && !dynamic_cast<PlayerOne*>(nearestObstacle.get())
	    && !dynamic_cast<CoopAI*>(nearestObstacle.get()))
	{
		if (shootDistance > _bulletDamageRadius + bulletOffset)
		{
			Shot();
		}
	}
}

void CoopAI::TickUpdate(const float deltaTime)
{
	// change dir when random time span left
	if (TimeUtils::IsCooldownFinish(_lastTimeTurn, _turnDurationMs))
	{
		_turnDurationMs = _distTurnRate(_gen);
		const int randDir = _distDirection(_gen);
		SetDirection(static_cast<Direction>(randDir));
		_lastTimeTurn = std::chrono::system_clock::now();
	}

	// move
	const auto pos = GetPos();
	_moveBeh->Move(deltaTime);

	// change dir it cant move
	if (pos == GetPos())
	{
		const int randDir = _distDirection(_gen);
		SetDirection(static_cast<Direction>(randDir));
	}

	// shot
	if (TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldownMs))
	{
		HandleLineOfSight(GetDirection());
	}
}

void CoopAI::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("CoopAIHit", author, fraction);

	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("CoopAIDied", author, fraction);
	}
}
