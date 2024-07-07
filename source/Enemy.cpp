#include "../headers/Enemy.h"
#include "../headers/ColliderCheck.h"
#include "../headers/CoopAI.h"
#include "../headers/EventSystem.h"
#include "../headers/LineOfSight.h"
#include "../headers/MoveLikeAIBeh.h"
#include "../headers/PlayerOne.h"
#include "../headers/PlayerTwo.h"
#include "../headers/Interfaces/IPickUpBonus.h"

#include <algorithm>
#include <chrono>

Enemy::Enemy(const Rectangle& rect, const int color, const int health, int* windowBuffer, const UPoint windowSize,
             Direction direction, float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
             std::shared_ptr<EventSystem> events, std::string name, std::string fraction,
             std::shared_ptr<BulletPool> bulletPool)
	: Tank{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       direction,
	       speed,
	       allObjects,
	       std::move(events),
	       std::make_shared<MoveLikeAIBeh>(this, allObjects),
	       std::move(bulletPool),
	       std::move(name),
	       std::move(fraction)},
	  _distDirection(0, 3), _distTurnRate(1, 5)
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	std::random_device rd;
	_gen = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count() + rd());

	Subscribe();

	_events->EmitEvent(_name + "_Spawn");
}

Enemy::~Enemy()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");
}

void Enemy::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		if (!_isActiveTeamFreeze)
		{
			this->TickUpdate(deltaTime);
		}
	});

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusTeamFreezeEnable",
			_name,
			[this](const std::string& author, const std::string& fraction, int bonusDurationTime)
			{
				if (fraction != _fraction)
				{
					this->_isActiveTeamFreeze = true;
				}
			});
	_events->AddListener<const std::string&, const std::string&>(
			"BonusTeamFreezeDisable",
			_name,
			[this](const std::string& author, const std::string& fraction)
			{
				if (fraction == _fraction)
				{
					this->_isActiveTeamFreeze = false;
				}
			});

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusHelmetEnable",
			_name,
			[this](const std::string& author, const std::string& fraction, int bonusDurationTime)
			{
				if (fraction == _fraction && author == _name)
				{
					this->_isActiveHelmet = true;
				}
			});
	_events->AddListener<const std::string&, const std::string&>(
			"BonusHelmetDisable",
			_name,
			[this](const std::string& author, const std::string& fraction)
			{
				if (fraction == _fraction && author == _name)
				{
					this->_isActiveHelmet = false;
				}
			});
}

void Enemy::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener("Draw", _name);

	_events->RemoveListener("DrawHealthBar", _name);

	_events->RemoveListener<const std::string&, const std::string&, int>("BonusTeamFreezeEnable", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusTeamFreezeDisable", _name);

	_events->RemoveListener<const std::string&, const std::string&, int>("BonusTeamFreezeEnable", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusTeamFreezeDisable", _name);
}

bool Enemy::IsPlayer(const std::weak_ptr<BaseObj>& obstacle)
{
	if (std::shared_ptr<BaseObj> isPlayerTeamSeen = obstacle.lock();
		dynamic_cast<PlayerOne*>(isPlayerTeamSeen.get())
		|| dynamic_cast<PlayerTwo*>(isPlayerTeamSeen.get())
		|| dynamic_cast<CoopAI*>(isPlayerTeamSeen.get()))
	{
		return true;
	}

	return false;
}

bool Enemy::IsBonus(const std::weak_ptr<BaseObj>& obstacle)
{
	if (std::shared_ptr<BaseObj> isBonusSeen = obstacle.lock(); dynamic_cast<IPickUpBonus*>(isBonusSeen.get()))
	{
		return true;
	}

	return false;
}

void Enemy::HandleLineOfSight(Direction dir)
{
	const FPoint bulletSize = {GetBulletWidth(), GetBulletHeight()};
	const FPoint bulletHalf = {bulletSize.x / 2.f, bulletSize.y / 2.f};
	LineOfSight lineOffSight(_shape, _windowSize, bulletHalf, _allObjects, this);

	const auto upSideObstacles = lineOffSight.GetUpSideObstacles();
	if (!upSideObstacles.empty())
	{
		if (IsPlayer(upSideObstacles.front()))
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

	const auto leftSideObstacles = lineOffSight.GetLeftSideObstacles();
	if (!leftSideObstacles.empty())
	{
		if (IsPlayer(leftSideObstacles.front()))
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

	const auto downSideObstacles = lineOffSight.GetDownSideObstacles();
	if (!downSideObstacles.empty())
	{
		if (IsPlayer(downSideObstacles.front()))
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

	const auto rightSideObstacles = lineOffSight.GetRightSideObstacles();
	if (!rightSideObstacles.empty())
	{
		if (IsPlayer(rightSideObstacles.front()))
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

	if (nearestObstacle && nearestObstacle.get() && nearestObstacle->GetIsDestructible()
	    && !dynamic_cast<Enemy*>(nearestObstacle.get()))
	{
		if (shootDistance > _bulletDamageAreaRadius + bulletOffset)
		{
			Shot();
		}
	}
}

void Enemy::TickUpdate(const float deltaTime)
{
	// change dir when random time span left
	if (IsTurnCooldownFinish())
	{
		_turnDuration = _distTurnRate(_gen);
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
	if (IsReloadFinish())
	{
		HandleLineOfSight(GetDirection());
		_lastTimeFire = std::chrono::system_clock::now();
	}
}

bool Enemy::IsTurnCooldownFinish() const
{
	const auto lastTimeTurnSec =
			std::chrono::duration_cast<std::chrono::seconds>(_lastTimeTurn.time_since_epoch()).count();
	const auto currentSec =
			std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
			.count();

	if (currentSec - lastTimeTurnSec >= _turnDuration)
	{
		return true;
	}

	return false;
}

void Enemy::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>("EnemyHit", author, fraction);

	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("EnemyDied", author, fraction);
	}
}
