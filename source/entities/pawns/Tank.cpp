#include "entities/pawns/Tank.h"
#include "components/EventSystem.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h"
#include "interfaces/IShootable.h"

Tank::Tank(PawnProperty pawnProperty, std::unique_ptr<IMoveBeh> moveBeh, std::shared_ptr<IShootable> shootingBeh,
           const BonusEffectProperty effects)
	: Pawn{std::move(pawnProperty), std::move(moveBeh)},
	  _shootingBeh{std::move(shootingBeh)},
	  _effects{effects}
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

	_events->AddListener("ClientReceived_" + _name + "OnCaliber", _name, [this]()
	{
		this->OnBonusCaliber(_name, _fraction);
	});
}

void Tank::SubscribeBonus()
{
	_events->AddListener<const std::string&, const bool>(
			"BonusTimerStatusChange", _name,
			[this](const std::string& fraction, const bool isActive)
			{
				this->OnBonusTimer(fraction, isActive);
			});

	_events->AddListener<const std::string&, const bool>(
			"BonusHelmetStatusChange", _name,
			[this](const std::string& name, const bool isActive)
			{
				this->OnBonusHelmet(name, isActive);
				if (_gameMode == PlayAsHost)
				{
					_events->EmitEvent<const std::string&>(
							isActive
								? "ServerSend_OnHelmetActivate"
								: "ServerSend_OnHelmetDeactivate",
							_name);
				}
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

	_events->AddListener<const std::string&, const std::string&>(
			"BonusCaliber", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusCaliber(author, fraction);
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
	_events->RemoveListener("ClientReceived_" + _name + "OnCaliber", _name);
}

void Tank::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const bool>("BonusTimerStatusChange", _name);
	_events->RemoveListener<const std::string&, const bool>("BonusHelmetStatusChange", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusGrenade", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusStar", _name);
	_events->RemoveListener<const std::string&, const std::string&>("BonusCaliber", _name);
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

void Tank::OnBonusTimer(const std::string& fraction, const bool isActive)
{
	if (fraction == _fraction)
	{
		_effects.isTimerActive = isActive;
	}
}

void Tank::OnBonusHelmet(const std::string& name, const bool isActive)
{
	if (_name == name)
	{
		_helmet.isActive = isActive;
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

void Tank::OnBonusCaliber(const std::string& author, const std::string& fraction)
{
	if (fraction == _fraction && author == _name)
	{
		SetHealth(GetHealth() + 50);
		if (_tier > 3)
		{
			return;
		}

		_tier += 3;

		_speed *= 1.30f;
		_bulletSpeed *= 1.30f;
		_bulletDamage += 45;
		_fireCooldown -= milliseconds{450};
		_bulletDamageRadius *= 1.75f;

		if (_gameMode == PlayAsHost)
		{
			_events->EmitEvent<const std::string&>("ServerSend_OnCaliber", author);
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
