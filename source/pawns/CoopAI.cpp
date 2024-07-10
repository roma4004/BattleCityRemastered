#include "../../headers/pawns/CoopAI.h"
#include "../../headers/EventSystem.h"
#include "../../headers/LineOfSight.h"
#include "../../headers/MoveLikeAIBeh.h"
#include "../../headers/interfaces/IPickupableBonus.h"
#include "../../headers/pawns/Enemy.h"
#include "../../headers/pawns/PlayerOne.h"
#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <chrono>

CoopAI::CoopAI(const Rectangle& rect, const int color, const int health, int* windowBuffer, const UPoint windowSize,
               const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
               const std::shared_ptr<EventSystem>& events, std::string name, std::string fraction,
               std::shared_ptr<BulletPool> bulletPool)
	: Tank{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::make_shared<MoveLikeAIBeh>(this, allObjects),
	       std::move(bulletPool),
	       std::move(name),
	       std::move(fraction)},
	  _distDirection(0, 3), _distTurnRate(1000/*ms*/, 5000/*ms*/)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	std::random_device rd;
	_gen = std::mt19937(static_cast<unsigned int>(
		std::chrono::high_resolution_clock::now().time_since_epoch().count() + rd()));

	Subscribe();

	_events->EmitEvent(_name + "_Spawn");
}

CoopAI::~CoopAI()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");
}

void CoopAI::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		// bonuses
		if (_isActiveTimer && TimeUtils::IsCooldownFinish(_activateTimeTimer, _cooldownTimer))
		{
			_isActiveTimer = false;
			_cooldownTimer = 0;
		}
		if (_isActiveHelmet && TimeUtils::IsCooldownFinish(_activateTimeHelmet, _cooldownHelmet))
		{
			_isActiveHelmet = false;
			_cooldownHelmet = 0;
		}

		if (!_isActiveTimer)
		{
			this->TickUpdate(deltaTime);
		}
	});

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusTimer",
			_name,
			[this](const std::string& /*author*/, const std::string& fraction, const int bonusDurationTimeMs)
			{
				if (fraction != _fraction)
				{
					this->_isActiveTimer = true;
					_cooldownTimer += bonusDurationTimeMs;
					_activateTimeTimer = std::chrono::system_clock::now();
				}
			});

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusHelmet",
			_name,
			[this](const std::string& author, const std::string& fraction, const int bonusDurationTimeMs)
			{
				if (fraction == _fraction && author == _name)
				{
					this->_isActiveHelmet = true;
					_cooldownHelmet += bonusDurationTimeMs;
					_activateTimeHelmet = std::chrono::system_clock::now();
				}
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusGrenade",
			_name,
			[this](const std::string& /*author*/, const std::string& fraction)
			{
				if (fraction != _fraction)
				{
					this->SetIsAlive(false);
				}
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusStar",
			_name,
			[this](const std::string& author, const std::string& fraction)
			{
				if (fraction == _fraction && author == _name)
				{
					this->SetHealth(GetHealth() + 50);
					if (_tier > 4)
					{
						return;
					}

					++this->_tier;

					this->SetSpeed(this->GetSpeed() * 1.10f);
					this->SetBulletSpeed(GetBulletSpeed() * 1.10f);
					this->SetBulletDamage(GetBulletDamage() + 15);
					this->SetFireCooldownMs(this->GetFireCooldownMs() - 150);
					this->SetBulletDamageAreaRadius(this->GetBulletDamageAreaRadius() * 1.25f);
				}
			});
}

void CoopAI::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);

	_events->RemoveListener("DrawHealthBar", _name);

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
	const FPoint bulletSize = {GetBulletWidth(), GetBulletHeight()};
	const FPoint bulletHalf = {bulletSize.x / 2.f, bulletSize.y / 2.f};
	LineOfSight lineOfSight(_shape, _windowSize, bulletHalf, _allObjects, this);

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
		if (shootDistance > _bulletDamageAreaRadius + bulletOffset)
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
		_lastTimeFire = std::chrono::system_clock::now();
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
