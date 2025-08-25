#include "entities/pawns/Tank.h"
#include "components/EventSystem.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/AnimationType.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h"
#include "interfaces/IShootable.h"

Tank::Tank(PawnProperty pawnProperty, std::unique_ptr<IMoveBeh> moveBeh, std::shared_ptr<IShootable> shootingBeh,
           const BonusEffectProperty effects, const bool enableByDefault)
	: Pawn{std::move(pawnProperty), std::move(moveBeh)},
	  _shootingBeh{std::move(shootingBeh)},
	  _effects{effects}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	if (enableByDefault)
	{
		Tank::Subscribe();
	}

	_events->EmitEvent("TankSpawn", _uuid);
}

Tank::~Tank()
{
	Tank::Unsubscribe();

	_events->EmitEvent("TankDied", _uuid);

	const std::string& basicString = _name;
	const ObjRectangle objRectangle = _rect;
	const int color = _color;
	_events->EmitEvent("AnimationCreate", AnimationType::Tank_Explosion, objRectangle, basicString, color);
}

void Tank::Subscribe()
{
	Pawn::Subscribe();

	_events->AddListener("DrawHealthBar", _nameWithUuid, [this]()
	{
		if (!_effects.isHelmetActive)
		{
			this->DrawHealthBar(this);
		}
	});

	if (_gameMode == GameMode::PlayAsClient)
	{
		SubscribeAsClient();
	}

	SubscribeBonus();
}

void Tank::SubscribeAsClient()
{
	_events->AddListener(
			"ClientReceived_" + _name + "Shot", _nameWithUuid, [this](const Direction dir, const buuid& uuid)
			{
				this->SetDirection(dir);
				this->Shot(uuid);
			});

	_events->AddListener("ClientReceived_" + _name + "OnHelmetActivate", _nameWithUuid, [this]()
	{
		this->_effects.isHelmetActive = true;
	});

	_events->AddListener("ClientReceived_" + _name + "OnHelmetDeactivate", _nameWithUuid, [this]()
	{
		this->_effects.isHelmetActive = false;
	});

	_events->AddListener("ClientReceived_" + _name + "OnStar", _nameWithUuid, [this]()
	{
		this->OnBonusStar(_name, _fraction);
	});

	_events->AddListener("ClientReceived_" + _name + "OnCaliber", _nameWithUuid, [this]()
	{
		this->OnBonusCaliber(_name, _fraction);
	});
}

void Tank::SubscribeBonus()
{
	_events->AddListener(
			"BonusTimerStatusChange", _nameWithUuid,
			[this](const std::string& fraction, const bool isActive)
			{
				this->OnBonusTimer(fraction, isActive);
			});

	_events->AddListener(
			"BonusHelmetStatusChange", _nameWithUuid,
			[this](const std::string& name, const bool isActive)
			{
				this->OnBonusHelmet(name, isActive);
				if (_gameMode == GameMode::PlayAsHost)
				{
					_events->EmitEvent(isActive ? "ServerSend_OnHelmetActivate" : "ServerSend_OnHelmetDeactivate",
					                   _nameWithUuid);
				}
			});

	_events->AddListener("BonusGrenade", _nameWithUuid, [this](const std::string& author, const std::string& fraction)
	{
		this->OnBonusGrenade(author, fraction);
	});

	_events->AddListener("BonusStar", _nameWithUuid, [this](const std::string& author, const std::string& fraction)
	{
		this->OnBonusStar(author, fraction);
	});

	_events->AddListener("BonusCaliber", _nameWithUuid, [this](const std::string& author, const std::string& fraction)
	{
		this->OnBonusCaliber(author, fraction);
	});
}

void Tank::Unsubscribe() const
{
	Pawn::Unsubscribe();

	_events->RemoveListener("DrawHealthBar", _nameWithUuid);

	if (_gameMode == GameMode::PlayAsClient)
	{
		UnsubscribeAsClient();
	}

	UnsubscribeBonus();
}

void Tank::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_" + _name + "Shot", _nameWithUuid);

	_events->RemoveListener("ClientReceived_" + _name + "OnHelmetActivate", _nameWithUuid);
	_events->RemoveListener("ClientReceived_" + _name + "OnHelmetDeactivate", _nameWithUuid);
	_events->RemoveListener("ClientReceived_" + _name + "OnStar", _nameWithUuid);
	_events->RemoveListener("ClientReceived_" + _name + "OnCaliber", _nameWithUuid);
}

void Tank::UnsubscribeBonus() const
{
	_events->RemoveListener("BonusTimerStatusChange", _nameWithUuid);
	_events->RemoveListener("BonusHelmetStatusChange", _nameWithUuid);
	_events->RemoveListener("BonusGrenade", _nameWithUuid);
	_events->RemoveListener("BonusStar", _nameWithUuid);
	_events->RemoveListener("BonusCaliber", _nameWithUuid);
}

void Tank::Disable() const
{
	Unsubscribe();
}

void Tank::Enable()
{
	Subscribe();
}

void Tank::TakeDamage(const int damage)
{
	if (!_effects.isHelmetActive)
	{
		Pawn::TakeDamage(damage);
	}
}

int Tank::GetTier() const { return _tier; }

void Tank::Shot(const buuid withUuid) const
{
	_lastTimeFire = std::chrono::system_clock::now();
	const buuid bulletUuid = _shootingBeh->Shot(withUuid);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_Shot", _name, GetDirection(), bulletUuid);
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

void Tank::DrawHealthBar(const BaseObj* obj) const
{
	_events->EmitEvent("DrawHealthBarObj", obj->GetRect(), obj->GetHealth(), obj->GetColor());
}

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
		_effects.isHelmetActive = isActive;
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

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_OnStar", author);
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

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_OnCaliber", author);
		}
	}
}

void Tank::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent("Statistics_TankHit", _name, author, fraction);

	if (GetHealth() < 1)
	{
		//TODO: move to event from statistic when last tank died
		_events->EmitEvent("Statistics_TankDied", _name, author, fraction);
	}
}
