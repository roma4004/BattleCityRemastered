#include "../../headers/pawns/PlayerTwo.h"
#include "../../headers/EventSystem.h"
#include "../../headers/behavior/MoveLikeTankBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/utils/TimeUtils.h"

#include <chrono>

PlayerTwo::PlayerTwo(const ObjRectangle& rect, const int color, const int health, int* windowBuffer,
                     const UPoint windowSize, const Direction direction, const float speed,
                     std::vector<std::shared_ptr<BaseObj>>* allObjects, const std::shared_ptr<EventSystem>& events,
                     std::string name, std::string fraction, std::unique_ptr<IInputProvider>& inputProvider,
                     std::shared_ptr<BulletPool> bulletPool, const bool isNetworkControlled,
                     const bool isHost, const int tankId)
	: Tank{rect,
	       color,
	       health,
	       windowBuffer,
	       windowSize,
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::make_shared<MoveLikeTankBeh>(this, allObjects),
	       std::make_shared<ShootingBeh>(this, windowBuffer, allObjects, events, std::move(bulletPool)),
	       std::move(name),
	       std::move(fraction),
	       tankId},
	  _inputProvider{std::move(inputProvider)},
	  _isNetworkControlled{isNetworkControlled},
	  _isHost{isHost}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();

	_events->EmitEvent(_name + "_Spawn");
}


PlayerTwo::~PlayerTwo()
{
	Unsubscribe();

	_events->EmitEvent(_name + "_Died");
}

void PlayerTwo::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });

	_events->AddListener("DrawHealthBar", _name, [this]() { this->DrawHealthBar(); });

	if (_isNetworkControlled)
	{
		_events->RemoveListener<const FPoint, const Direction>("Net_" + _name + "_NewPos", _name);

		_events->AddListener<const FPoint, const Direction>(
				"Net_" + _name + "_NewPos",
				_name,
				[this](const FPoint newPos, const Direction direction)
				{
					this->SetPos(newPos);
					this->SetDirection(direction);
				});

		_events->AddListener<const int>("Net_" + _name + "_NewHealth", _name, [this](const int health)
		{
			this->SetHealth(health);
		});

		_events->AddListener<const Direction>("Net_" + _name + "_Shot", _name, [this](const Direction direction)
		{
			SetDirection(direction);
			this->Shot();
		});

		if (!_isHost)
		{
			return;
		}
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

void PlayerTwo::Unsubscribe() const
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

		if (!_isHost)
		{
			return;
		}
	}

	_events->RemoveListener<const float>("TickUpdate", _name);

	_events->RemoveListener<const std::string&, const std::string&, int>("BonusTimer", _name);
	_events->RemoveListener<const std::string&, const std::string&, int>("BonusHelmet", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusGrenade", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusStar", _name);
}

void PlayerTwo::MoveTo(const float deltaTime, const Direction direction)
{
	SetDirection(direction);
	_moveBeh->Move(deltaTime);

	if (/*!_isNetworkControlled ||*/ _isHost)
	{
		_events->EmitEvent<const std::string, const std::string, const FPoint, const Direction>(
				"_NewPos", "Player" + std::to_string(GetTankId()), "_NewPos", GetPos(), GetDirection());
	}
}

void PlayerTwo::TickUpdate(const float deltaTime)
{
	if (_inputProvider == nullptr)
	{
		return;
	}

	const auto playerKeys = _inputProvider->GetKeysStats();

	// move
	if (playerKeys.left)
	{
		MoveTo(deltaTime, LEFT);
	}
	else if (playerKeys.right)
	{
		MoveTo(deltaTime, RIGHT);
	}
	else if (playerKeys.up)
	{
		MoveTo(deltaTime, UP);
	}
	else if (playerKeys.down)
	{
		MoveTo(deltaTime, DOWN);
	}

	// shot
	if (playerKeys.shot && TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldownMs))
	{
		this->Shot();
		_events->EmitEvent<const Direction>(_name + "_Shot", GetDirection());
		_lastTimeFire = std::chrono::system_clock::now();
	}
}

void PlayerTwo::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&>(_name + "Hit", author, fraction);

	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>(_name + "Died", author, fraction);
	}
}

void PlayerTwo::TakeDamage(const int damage)
{
	Tank::TakeDamage(damage);

	if (/*!_isNetworkControlled ||*/ _isHost)
	{
		_events->EmitEvent<const std::string, const std::string, const int>(
				"SendHealth", GetName(), "_NewHealth", GetHealth());
	}
}
