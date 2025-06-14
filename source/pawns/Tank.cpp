#include "../../headers/pawns/Tank.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/interfaces/IMoveBeh.h"
#include "../../headers/interfaces/IShootable.h"
#include "../../headers/pawns/PawnProperty.h"
#include "../../headers/utils/TimeUtils.h"

Tank::Tank(PawnProperty pawnProperty, std::unique_ptr<IMoveBeh> moveBeh, std::shared_ptr<IShootable> shootingBeh)
	: Pawn{std::move(pawnProperty), std::move(moveBeh)},
	  _shootingBeh{std::move(shootingBeh)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Tank::Subscribe();

	_events->EmitEvent<const buuid&>("TankSpawn", _uuid);
}

Tank::~Tank()
{
	Tank::Unsubscribe();

	_events->EmitEvent<const buuid&>("TankDied", _uuid);
}

void Tank::Subscribe()
{
	_events->AddListener("DrawHealthBar", _nameWithUuid, [this]()
	{
		if (!_helmet.isActive)
		{
			this->DrawHealthBar(this);
		}
	});

	if (_gameMode == PlayAsClient)
	{
		SubscribeAsClient();
	}

	SubscribeBonus();
}

void Tank::SubscribeAsClient()
{
	_events->AddListener<const Direction, const buuid&>(
			"ClientReceived_" + _name + "Shot", _name, [this](const Direction dir, const buuid& uuid)
			{
				this->SetDirection(dir);
				this->Shot(uuid);
			});

	_events->AddListener("ClientReceived_" + _name + "OnHelmetActivate", _name, [this]()
	{
		this->_helmet.isActive = true;
	});

	_events->AddListener("ClientReceived_" + _name + "OnHelmetDeactivate", _name, [this]()
	{
		this->_helmet.isActive = false;
	});

	_events->AddListener("ClientReceived_" + _name + "OnStar", _name, [this]()
	{
		this->OnBonusStar(_name, _fraction);
	});
}

void Tank::SubscribeBonus()
{
	_events->AddListener<const std::string&, const std::string&, const milliseconds>(
			"BonusTimer", _name,
			[this](const std::string& /*author*/, const std::string& fraction, const milliseconds duration)
			{
				this->OnBonusTimer(fraction, duration);
			});

	_events->AddListener<const std::string&, const std::string&, const milliseconds>(
			"BonusHelmet", _name,
			[this](const std::string& author, const std::string& fraction, const milliseconds duration)
			{
				this->OnBonusHelmet(author, fraction, duration);
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusGrenade", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusGrenade(author, fraction);
			});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusStar", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusStar(author, fraction);
			});
}

void Tank::Unsubscribe() const
{
	_events->RemoveListener("DrawHealthBar", _nameWithUuid);

	if (_gameMode == PlayAsClient)
	{
		UnsubscribeAsClient();
	}

	UnsubscribeBonus();
}

void Tank::UnsubscribeAsClient() const
{
	_events->RemoveListener<const Direction, const buuid&>("ClientReceived_" + _name + "Shot", _name);

	_events->RemoveListener("ClientReceived_" + _name + "OnHelmetActivate", _name);
	_events->RemoveListener("ClientReceived_" + _name + "OnHelmetDeactivate", _name);
	_events->RemoveListener("ClientReceived_" + _name + "OnStar", _name);
}

void Tank::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const std::string&, const milliseconds>("BonusTimer", _name);
	_events->RemoveListener<const std::string&, const std::string&, const milliseconds>("BonusHelmet", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusGrenade", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusStar", _name);
}

void Tank::TickUpdate(const float /*deltaTime*/)
{
	if (_timer.isActive && TimeUtils::IsCooldownFinish(_timer.activateTime, _timer.cooldown))
	{
		_timer.isActive = false;
	}

	if (_helmet.isActive && TimeUtils::IsCooldownFinish(_helmet.activateTime, _helmet.cooldown))
	{
		_helmet.isActive = false;

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&>("ServerSend_OnHelmetDeactivate", _name);
		}
	}
}

void Tank::TakeDamage(const int damage)
{
	if (!_helmet.isActive)
	{
		Pawn::TakeDamage(damage);

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&, const int, const buuid&>(
					"ServerSend_Health", _name, GetHealth(), _uuid);
		}
	}
}

int Tank::GetTier() const { return _tier; }

void Tank::Shot(const buuid withUuid) const
{
	_lastTimeFire = std::chrono::system_clock::now();
	const buuid bulletUuid = _shootingBeh->Shot(withUuid);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const Direction, const buuid&>(
				"ServerSend_Shot", _name, GetDirection(), bulletUuid);
	}
}

float Tank::GetBulletWidth() const { return _bulletSize.x; }

void Tank::SetBulletWidth(const float bulletWidth) { _bulletSize.x = bulletWidth; }

float Tank::GetBulletHeight() const { return _bulletSize.y; }

void Tank::SetBulletHeight(const float bulletHeight) { _bulletSize.y = bulletHeight; }

float Tank::GetBulletSpeed() const { return _bulletSpeed; }

void Tank::SetBulletSpeed(const float bulletSpeed) { _bulletSpeed = bulletSpeed; }

int Tank::GetBulletDamage() const { return _bulletDamage; }

void Tank::SetBulletDamage(const int bulletDamage) { _bulletDamage = bulletDamage; }

double Tank::GetBulletDamageRadius() const { return _bulletDamageRadius; }

void Tank::SetBulletDamageRadius(const double bulletDamageRadius) { _bulletDamageRadius = bulletDamageRadius; }

void Tank::DrawHealthBar(const BaseObj* obj) const { _events->EmitEvent<const BaseObj*>("DrawHealthBarObj", obj); }

void Tank::OnBonusTimer(const std::string& fraction, const milliseconds duration)
{
	if (fraction != _fraction)
	{
		const auto cooldown = _timer.cooldown += duration;
		_timer = {true, cooldown, std::chrono::system_clock::now()};
	}
}

void Tank::OnBonusHelmet(const std::string& author, const std::string& fraction, const milliseconds duration)
{
	if (fraction == _fraction && author == _name)
	{
		const auto cooldown = _helmet.cooldown += duration;
		_helmet = {true, cooldown, std::chrono::system_clock::now()};

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&>("ServerSend_OnHelmetActivate", author);
		}
	}
}

void Tank::OnBonusGrenade(const std::string& /*author*/, const std::string& fraction)
{
	if (fraction != _fraction)
	{
		TakeDamage(GetHealth());
	}
}

void Tank::OnBonusStar(const std::string& author, const std::string& fraction)
{
	if (fraction == _fraction && author == _name)
	{
		SetHealth(GetHealth() + 50);
		if (_tier > 4)
		{
			return;
		}

		++_tier;

		_speed *= 1.10f;
		_bulletSpeed *= 1.10f;
		_bulletDamage += 15;
		_fireCooldown -= milliseconds{150};
		_bulletDamageRadius *= 1.25f;

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&>("ServerSend_OnStar", author);
		}
	}
}

void Tank::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
			"Statistics_TankHit", _name, author, fraction);

	if (GetHealth() < 1)
	{
		//TODO: move to event from statistic when last tank died
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"Statistics_TankDied", _name, author, fraction);
	}
}
