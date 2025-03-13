#include "../../headers/pawns/Enemy.h"
#include "../../headers/EventSystem.h"
#include "../../headers/LineOfSight.h"
#include "../../headers/behavior/MoveLikeAIBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/interfaces/IPickupableBonus.h"
#include "../../headers/pawns/CoopAI.h"
#include "../../headers/pawns/PlayerOne.h"
#include "../../headers/pawns/PlayerTwo.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <chrono>

//TODO: if enemy see bullets they should tru or prioritize move aside
Enemy::Enemy(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
             const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
             const std::shared_ptr<EventSystem>& events, std::string name, std::string fraction,
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
	  _distDirection(0, 3),
	  _distTurnRate(1000/*ms*/, 5000/*ms*/),
	  _lastTimeTurn{std::chrono::system_clock::now()}
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

Enemy::~Enemy()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent("ServerSend_" + _name + "_Died");
	}
}

void Enemy::Subscribe()
{
	_gameMode == PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();

	SubscribeBonus();
}

void Enemy::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		// bonuses
		_timer.UpdateBonus();
		_helmet.UpdateBonus();

		if (!this->_timer.isActive)
		{
			this->TickUpdate(deltaTime);
		}
	});
}

void Enemy::SubscribeAsClient()
{
	_events->AddListener<const FPoint, const Direction>(
			"ClientReceived_" + _name /*TODO: + std::to_string(GetId())*/ + "Pos", _name,
			[this](const FPoint newPos, const Direction direction)
			{
				this->SetPos(newPos);
				this->SetDirection(direction);
			});

	_events->AddListener<const Direction>(
			"ClientReceived_" + _name + "Shot", _name, [this](const Direction direction)
			{
				this->SetDirection(direction);
				this->Shot();
			});

	_events->AddListener<const int>(
			"ClientReceived_" + _name + "Health", _name, [this](const int health)
			{
				this->SetHealth(health);
			});

	_events->AddListener<const std::string>(
			"ClientReceived_" + _name + "TankDied", _name, [this](const std::string whoDied)
			{
				this->SetIsAlive(false);
			});
}

void Enemy::SubscribeBonus()
{
	_events->AddListener<const std::string&, const std::string&, std::chrono::milliseconds>(
			"BonusTimer", _name,
			[this](const std::string& /*author*/, const std::string& fraction,
			       const std::chrono::milliseconds bonusDurationTime)
			{
				if (fraction != this->_fraction)
				{
					const auto cooldown = this->_timer.cooldown += bonusDurationTime;
					this->_timer = {true, cooldown, std::chrono::system_clock::now()};
				}
			});

	_events->AddListener<const std::string&, const std::string&, std::chrono::milliseconds>(
			"BonusHelmet", _name,
			[this](const std::string& author, const std::string& fraction, const std::chrono::milliseconds bonusDurationTime)
			{
				if (fraction == this->_fraction && author == this->_name)
				{
					const auto cooldown = this->_helmet.cooldown += bonusDurationTime;
					this->_helmet = {true, cooldown, std::chrono::system_clock::now()};
				}
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusGrenade", _name, [this](const std::string& /*author*/, const std::string& fraction)
			{
				if (fraction != this->_fraction)
				{
					this->TakeDamage(GetHealth());
				}
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusStar", _name, [this](const std::string& author, const std::string& fraction)
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
					this->SetFireCooldown(this->GetFireCooldown() - std::chrono::milliseconds{150});
					this->SetBulletDamageRadius(this->GetBulletDamageRadius() * 1.25f);
				}
			});
	//TOOD: subscribe other bonuses
}

void Enemy::Unsubscribe() const
{
	_gameMode == PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();

	UnsubscribeBonus();
}

void Enemy::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _name);
}

void Enemy::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const Direction>("ClientReceived_" + _name + "Pos", _name);
	_events->RemoveListener<const Direction>("ClientReceived_" + _name + "Shot", _name);
	_events->RemoveListener<const int>("ClientReceived_" + _name + "Health", _name);
	_events->RemoveListener<const std::string>("ClientReceived_" + _name + "TankDied", _name);
}

void Enemy::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const std::string&, std::chrono::milliseconds>("BonusTimer", _name);
	_events->RemoveListener<const std::string&, const std::string&, std::chrono::milliseconds>("BonusHelmet", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusGrenade", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusStar", _name);
}

bool Enemy::IsPlayer(const std::weak_ptr<BaseObj>& obstacle)
{
	if (const std::shared_ptr<BaseObj> isPlayerTeamSeen = obstacle.lock();
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
	if (const std::shared_ptr<BaseObj> isBonusSeen = obstacle.lock();
		dynamic_cast<IPickupableBonus*>(isBonusSeen.get()))
	{
		return true;
	}

	return false;
}

void Enemy::HandleLineOfSight(const Direction dir)
{
	const FPoint bulletSize = {.x = GetBulletWidth(), .y = GetBulletHeight()};
	const FPoint bulletHalf = {.x = bulletSize.x / 2.f, .y = bulletSize.y / 2.f};
	LineOfSight lineOffSight(_shape, _window->size, bulletHalf, _allObjects, this);

	const auto& upSideObstacles = lineOffSight.GetUpSideObstacles();
	if (!upSideObstacles.empty())
	{
		const auto nearestObstacle = upSideObstacles.front();
		if (IsPlayer(nearestObstacle))
		{
			SetDirection(UP);
			Shot();

			return;
		}

		if (IsBonus(nearestObstacle))
		{
			SetDirection(UP);

			return;
		}
	}

	const auto& leftSideObstacles = lineOffSight.GetLeftSideObstacles();
	if (!leftSideObstacles.empty())
	{
		const auto nearestObstacle = leftSideObstacles.front();
		if (IsPlayer(nearestObstacle))
		{
			SetDirection(LEFT);
			Shot();

			return;
		}

		if (IsBonus(nearestObstacle))
		{
			SetDirection(LEFT);

			return;
		}
	}

	const auto& downSideObstacles = lineOffSight.GetDownSideObstacles();
	if (!downSideObstacles.empty())
	{
		const auto nearestObstacle = downSideObstacles.front();
		if (IsPlayer(nearestObstacle))
		{
			SetDirection(DOWN);
			Shot();

			return;
		}

		if (IsBonus(nearestObstacle))
		{
			SetDirection(DOWN);

			return;
		}
	}

	const auto& rightSideObstacles = lineOffSight.GetRightSideObstacles();
	if (!rightSideObstacles.empty())
	{
		const auto nearestObstacle = rightSideObstacles.front();
		if (IsPlayer(nearestObstacle))
		{
			SetDirection(RIGHT);
			Shot();

			return;
		}

		if (IsBonus(nearestObstacle))
		{
			SetDirection(RIGHT);

			return;
		}
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

	if (nearestObstacle && nearestObstacle.get() && nearestObstacle->GetIsDestructible()
	    && !dynamic_cast<Enemy*>(nearestObstacle.get()))
	{
		if (shootDistance > _bulletDamageRadius + bulletOffset)//TODO: cover this by test
		{
			Shot();
		}
	}

}

void Enemy::TickUpdate(const float deltaTime)
{
	// change dir when random time span left
	if (TimeUtils::IsCooldownFinish(_lastTimeTurn, _turnDuration))
	{
		_turnDuration = std::chrono::milliseconds(_distTurnRate(_gen));
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

	//NOTE: move synchronize to client
	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const Direction>(
				"ServerSend_Pos", "Enemy" + std::to_string(GetId()), GetPos(), GetDirection());
	}

	// shot
	if (TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		HandleLineOfSight(GetDirection());
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
