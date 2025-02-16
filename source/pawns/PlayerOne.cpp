#include "../../headers/pawns/PlayerOne.h"
#include "../../headers/EventSystem.h"
#include "../../headers/behavior/MoveLikeTankBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/utils/TimeUtils.h"

#include <chrono>

PlayerOne::PlayerOne(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<int[]> windowBuffer,
                     UPoint windowSize, const Direction direction, const float speed,
                     std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
                     std::string name, std::string fraction, std::unique_ptr<IInputProvider> inputProvider,
                     std::shared_ptr<BulletPool> bulletPool, const bool isNetworkControlled, const int tankId)
	: Tank{rect,
	       color,
	       health,
	       std::move(windowBuffer),
	       std::move(windowSize),
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::make_unique<MoveLikeTankBeh>(this, allObjects),
	       std::make_shared<ShootingBeh>(this, allObjects, events, std::move(bulletPool)),
	       std::move(name),
	       std::move(fraction),
	       tankId},
	  _inputProvider{std::move(inputProvider)},
	  _isNetworkControlled{isNetworkControlled}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();

	_events->EmitEvent(_name + "_Spawn");
}

PlayerOne::~PlayerOne()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");
}

void PlayerOne::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });

	if (_isNetworkControlled)
	{
		_events->AddListener<const FPoint, const Direction>(
				"Net_" + _name + "_NewPos",
				_name,
				[this](const FPoint newPos, const Direction direction)
				{
					this->SetPos(newPos);
					this->SetDirection(direction);
				});

		_events->AddListener<const Direction>("Net_" + _name + "_Shot", _name, [this](const Direction direction)
		{
			this->SetDirection(direction);
			this->Shot();
		});

		_events->AddListener<const int>("Net_" + _name + "_NewHealth", _name, [this](const int health)
		{
			this->SetHealth(health);
		});

		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		// bonuses disable timer
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

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusTimer",
			_name,
			[this](const std::string& /*author*/, const std::string& fraction, const int bonusDurationTime)
			{
				if (fraction != _fraction)
				{
					this->_isActiveTimer = true;
					_cooldownTimer += bonusDurationTime;
					_activateTimeTimer = std::chrono::system_clock::now();
				}
			});

	_events->AddListener<const std::string&, const std::string&, int>(
			"BonusHelmet",
			_name,
			[this](const std::string& author, const std::string& fraction, const int bonusDurationTime)
			{
				if (fraction == _fraction && author == _name)
				{
					this->_isActiveHelmet = true;
					_cooldownHelmet += bonusDurationTime;
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
					this->TakeDamage(GetHealth());
				}
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusStar",
			_name,
			[this](const std::string& author, const std::string& fraction)
			{
				if (fraction == _fraction && author == _name)
				{
					this->SetHealth(this->GetHealth() + 50);
					if (_tier > 4)
					{
						return;
					}

					++this->_tier;

					this->SetSpeed(this->GetSpeed() * 1.10f);
					this->SetBulletSpeed(this->GetBulletSpeed() * 1.10f);
					this->SetBulletDamage(this->GetBulletDamage() + 15);
					this->SetFireCooldownMs(this->GetFireCooldownMs() - 150);
					this->SetBulletDamageAreaRadius(this->GetBulletDamageAreaRadius() * 1.25f);
				}
			});
}

void PlayerOne::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("Draw", _name);

	_events->RemoveListener("DrawHealthBar", _name);

	if (_isNetworkControlled)
	{
		_events->RemoveListener<const FPoint, const Direction>("Net_" + _name + "_NewPos", _name);

		_events->RemoveListener<const Direction>("Net_" + _name + "_Shot", _name);

		_events->RemoveListener<const int>("Net_" + _name + "_NewHealth", _name);

		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener<const std::string&, const std::string&, int>("BonusTimer", _name);
	_events->RemoveListener<const std::string&, const std::string&, int>("BonusHelmet", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusGrenade", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusStar", _name);
}

void PlayerOne::MoveTo(const float deltaTime, const Direction direction)
{
	SetDirection(direction);
	_moveBeh->Move(deltaTime);

	if (!_isNetworkControlled)
	{
		_events->EmitEvent<const std::string, const std::string, const FPoint, const Direction>(
				"_NewPos", "Player" + std::to_string(GetTankId()), "_NewPos", GetPos(), GetDirection());
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

		if (!_isNetworkControlled)
		{
			_events->EmitEvent<const Direction>(_name + "_Shot", GetDirection());
		}

		_lastTimeFire = std::chrono::system_clock::now();
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

	if (!_isNetworkControlled)
	{
		_events->EmitEvent<const std::string, const std::string, const int>(
				"SendHealth", GetName(), "_NewHealth", GetHealth());
	}
}
