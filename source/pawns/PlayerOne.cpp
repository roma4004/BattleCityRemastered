#include "../../headers/pawns/PlayerOne.h"
#include "../../headers/EventSystem.h"
#include "../../headers/behavior/MoveLikeTankBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/utils/TimeUtils.h"

#include <chrono>

PlayerOne::PlayerOne(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
                     const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                     std::shared_ptr<EventSystem> events, std::string name, std::string fraction,
                     std::unique_ptr<IInputProvider> inputProvider, std::shared_ptr<BulletPool> bulletPool,
                     const GameMode gameMode, const int id)
	: Tank{rect,
	       color,
	       health,
	       std::move(window),
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::make_unique<MoveLikeTankBeh>(this, allObjects),
	       std::make_shared<ShootingBeh>(this, allObjects, events, std::move(bulletPool)),
	       std::move(name),
	       std::move(fraction),
	       gameMode,
	       id},
	  _gameMode{gameMode},
	  _inputProvider{std::move(inputProvider)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();

	events->EmitEvent(_name + "_Spawn");
}

PlayerOne::~PlayerOne()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");
}

void PlayerOne::Subscribe()
{
	Tank::Subscribe();

	_gameMode == PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();

	SubscribeBonus();
}

void PlayerOne::SubscribeAsHost()
{
	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		// TODO: generalize checking is bonus effect active and cooldown
		// bonuses disable timer
		if (this->_isActiveTimer && TimeUtils::IsCooldownFinish(this->_activateTimeTimer, this->_cooldownTimer))
		{
			this->_isActiveTimer = false;
			this->_cooldownTimer = 0;
		}

		if (this->_isActiveHelmet && TimeUtils::IsCooldownFinish(this->_activateTimeHelmet, this->_cooldownHelmet))
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

void PlayerOne::SubscribeAsClient()
{
	_events->AddListener<const FPoint, const Direction>(
			"ClientReceived_" + _name + "Pos", _name, [this](const FPoint newPos, const Direction direction)
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
			"ClientReceived_" + _name + "TankDied", _name, [this](const std::string/* whoDied*/)
			{
				this->SetIsAlive(false);
			});
}

void PlayerOne::SubscribeBonus()
{
	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusTimer", _name,
			[this](const std::string& /*author*/, const std::string& fraction, const int bonusDurationTime)
			{
				if (fraction != this->_fraction)
				{
					this->_isActiveTimer = true;
					this->_cooldownTimer += bonusDurationTime;
					this->_activateTimeTimer = std::chrono::system_clock::now();
				}
			});

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusHelmet", _name,
			[this](const std::string& author, const std::string& fraction, const int bonusDurationTime)
			{
				if (fraction == this->_fraction && author == this->_name)
				{
					this->_isActiveHelmet = true;
					this->_cooldownHelmet += bonusDurationTime;
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

void PlayerOne::Unsubscribe() const
{
	Tank::Unsubscribe();

	_gameMode == PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();

	UnsubscribeBonus();
}

void PlayerOne::UnsubscribeAsHost() const
{
	_events->RemoveListener<const float>("TickUpdate", _name);
}

void PlayerOne::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const Direction>("ClientReceived_" + _name + "Pos", _name);
	_events->RemoveListener<const Direction>("ClientReceived_" + _name + "Shot", _name);
	_events->RemoveListener<const std::string>("ClientReceived_" + _name + "TankDied", _name);
	_events->RemoveListener<const int>("ClientReceived_" + _name + "Health", _name);
}

void PlayerOne::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const std::string&, int>("BonusTimer", _name);
	_events->RemoveListener<const std::string&, const std::string&, int>("BonusHelmet", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusGrenade", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusStar", _name);
}

void PlayerOne::MoveTo(const float deltaTime, const Direction direction)
{
	SetDirection(direction);
	_moveBeh->Move(deltaTime);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const Direction>(
				"ServerSend_Pos", "Player" + std::to_string(GetId()), GetPos(), GetDirection());
	}
}

void PlayerOne::TickUpdate(const float deltaTime)
{
	if (_inputProvider == nullptr)
	{
		return;
	}

	const auto playerKeys = _inputProvider->GetKeysStats();

	// move
	if (playerKeys.up)
	{
		MoveTo(deltaTime, UP);
	}
	else if (playerKeys.left)
	{
		MoveTo(deltaTime, LEFT);
	}
	else if (playerKeys.down)
	{
		MoveTo(deltaTime, DOWN);
	}
	else if (playerKeys.right)
	{
		MoveTo(deltaTime, RIGHT);
	}

	// shot
	if (playerKeys.shot && TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldownMs))
	{
		Shot();
	}
}

void PlayerOne::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>(_name + "Hit", author, fraction);

	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>(_name + "Died", author, fraction);
	}
}

void PlayerOne::TakeDamage(const int damage)
{
	Tank::TakeDamage(damage);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string, const int>("ServerSend_Health", GetName(), GetHealth());
	}
}
