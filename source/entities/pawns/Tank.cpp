#include "entities/pawns/Tank.h"
#include "application/GameConfig.h"
#include "behavior/MoveLikeTankBeh.h"
#include "behavior/ShootingBeh.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/StatisticsEvents.h"
#include "entities/BulletCalibre.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/ColliderUtils.h"

Tank::Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, GameConfig& gameConfig,
		   const bool enableByDefault)
	: Pawn{std::move(pawnProperty), gameConfig}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	_moveBeh = std::make_unique<MoveLikeTankBeh>(_rect, _dir, _speed, _uuid, _gameConfig.windowSize, _name, _fraction,
												 _allObjects, _effects, gameConfig);
	_calibre = BulletCalibre{.speed = 300.f,
							 .damage = 15,
							 .damageRadius = 18.f,
							 .tier = _tier,
							 .size{.x = 9.f, .y = 9.f}};
	ApplyScaleToCalibre(gameConfig.scaleFactor);

	_shootingBeh = std::make_shared<ShootingBeh>(_rect, _dir, _uuid, _gameConfig.windowSize, _name, _fraction,
												 _allObjects, bulletPool, _calibre, _events);

	if (enableByDefault)
	{
		Tank::Subscribe();
	}

	// NOTE: should be in constructor to be able to enable by replication
	if (_gameMode == GameMode::PlayAsClient)
	{
		_events->AddListener(
				"ClientReceived_" + _name + "OnTankOnOff", _nameWithUuid,
				[this](const buuid uuid, const bool isEnable)
				{
					this->OnClientTankOnOff(uuid, isEnable);
				});

		_events->AddListener(
				"ClientReceived_" + _name + "Pos",
				_nameWithUuid,
				[this](const FPoint newPos, const Direction dir, const buuid& uuid)
				{
					this->OnClientChangePos(newPos, dir, uuid);
				});
	}

	_events->AddListener(
			"BonusTimer_ReApplyOnSpawn", _nameWithUuid,
			[this](const bool isEnabled, const std::string& name)
			{
				if (name == this->_name)
				{
					if (isEnabled)
					{
						this->UnsubscribeTickUpdate();
					}
					else
					{
						this->SubscribeTickUpdate();
					}
				}
			});

	_events->EmitEvent("TankSpawn", _uuid);
}

Tank::~Tank()
{
	Tank::Unsubscribe();

	_events->EmitEvent("TankDied", _uuid);

	_events->EmitEvent("AnimationCreateTankExplosion", AnimationCreateExplosionEvent{_rect, _name});
}

void Tank::Subscribe()
{
	Pawn::Subscribe();

	_events->AddListener("PostDraw", _nameWithUuid, [this]()
	{
		if (this->_effects.isHelmetActive || this->_effects.isTouchTheBushes)
		{
			return;
		}

		this->_events->EmitEvent("RenderHealthBar", RenderHealthBarEvent{this->GetRect(), this->GetHealth()});
	});

	_events->AddListener("ScaleFactorChangedTo", _name, [this](const float newScale)
	{
		this->ApplyScaleToCalibre(newScale);
	});

	if (_gameMode == GameMode::PlayAsClient)
	{
		SubscribeAsClient();
	}

	SubscribeBonus();
}

void Tank::SubscribeAsClient()
{
	//TODO: remove + _name +  from eventName
	//TODO: rename ClientReceived_ to ClientIn
	//TODO: reduce number of "ClientReceived_" overloading if we can use just direct local event
	//TODO: refactor to ClientReceived_ "Shot" to just "Shot" and move bot timers to handle outside bot tank,
	_events->AddListener(
			"ClientReceived_" + _name + "Shot", _nameWithUuid, [this](const Direction dir, const buuid& uuid)
			{
				this->SetDirection(dir);
				this->Shot(uuid);
			});

	_events->AddListener("ClientReceived_" + _name + "BonusHelmet_Pickup", _nameWithUuid, [this](const bool isActive)
	{
		this->OnBonusHelmet(this->_name, isActive);
	});

	_events->AddListener("ClientReceived_" + _name + "BonusStar_Pickup", _nameWithUuid, [this]()
	{
		this->OnBonusStar(this->_name);
	});

	_events->AddListener("ClientReceived_" + _name + "BonusCaliber_Pickup", _nameWithUuid, [this]()
	{
		this->OnBonusCaliber(this->_name);
	});
}

void Tank::SubscribeBonus()
{
	_events->AddListener(
			"BonusTimer_StatusChange", _nameWithUuid,
			[this](const std::string& fraction, const bool isActive)
			{
				this->OnBonusTimer(fraction, isActive);
			});

	_events->AddListener(
			"BonusHelmet_StatusChange", _nameWithUuid,
			[this](const std::string& name, const bool isActive)
			{
				this->OnBonusHelmet(name, isActive);
			});

	_events->AddListener(
			"BonusGrenade_Pickup", _nameWithUuid,
			[this](const StatisticsAttributionEvent& event)
			{
				this->OnBonusGrenade(event.fraction);
			});

	_events->AddListener(
			"BonusStar_Pickup", _nameWithUuid,
			[this](const StatisticsAttributionEvent& event)
			{
				this->OnBonusStar(event.author);
			});

	_events->AddListener(
			"BonusCaliber_Pickup", _nameWithUuid,
			[this](const StatisticsAttributionEvent& event)
			{
				this->OnBonusCaliber(event.author);
			});
}

void Tank::Unsubscribe() const
{
	Pawn::Unsubscribe();
	_events->RemoveAllListeners(_nameWithUuid);
}

void Tank::Enable()
{
	Subscribe();

	if (_gameMode == GameMode::PlayAsHost)
	{
		constexpr bool isEnable = true;
		_events->EmitEvent("ServerSend_OnTankOnOff", _uuid, isEnable, _name);
	}
}

void Tank::Disable() const
{
	Unsubscribe();

	if (_gameMode == GameMode::PlayAsHost)
	{
		constexpr bool isEnable = false;
		_events->EmitEvent("ServerSend_OnTankOnOff", _uuid, isEnable, _name);
	}
}

void Tank::TakeDamage(const unsigned int damage, const std::string& damageAuthor, const std::string& damageFraction)
{
	if (!_effects.isHelmetActive)
	{
		Pawn::TakeDamage(damage, damageAuthor, damageFraction);
	}
}

unsigned int Tank::GetTier() const { return _tier; }

void Tank::Shot(const buuid withUuid)
{
	const buuid bulletUuid = _shootingBeh->Shot(withUuid);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_Shot", _name, GetDirection(), bulletUuid);
	}

	_shootTimer.Reset();
}

float Tank::GetBulletWidth() const { return _calibre.size.x; }

void Tank::SetBulletWidth(const float bulletWidth) { _calibre.size.x = bulletWidth; }

float Tank::GetBulletHeight() const { return _calibre.size.y; }

void Tank::SetBulletHeight(const float bulletHeight) { _calibre.size.y = bulletHeight; }

float Tank::GetBulletSpeed() const { return _calibre.speed; }

void Tank::SetBulletSpeed(const float bulletSpeed) { _calibre.speed = bulletSpeed; }

int Tank::GetBulletDamage() const { return _calibre.damage; }

void Tank::SetBulletDamage(const int bulletDamage) { _calibre.damage = bulletDamage; }

double Tank::GetBulletDamageRadius() const { return _calibre.damageRadius; }

void Tank::SetBulletDamageRadius(const double bulletDamageRadius) { _calibre.damageRadius = bulletDamageRadius; }

void Tank::OnBonusTimer(const std::string& fraction, const bool isActive)
{
	if (fraction == _fraction)
	{
		if (isActive)
		{
			UnsubscribeTickUpdate();
		}
		else
		{
			SubscribeTickUpdate();
		}
	}
}

void Tank::OnBonusHelmet(const std::string& name, const bool isActive)
{
	if (_name == name)
	{
		_effects.isHelmetActive = isActive;

		_events->EmitEvent("BonusHelmet_AnimationChange", BonusHelmetAnimationChangeEvent{_name, isActive});

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_BonusHelmet_Pickup", _name, isActive);
		}
	}
}

void Tank::OnBonusGrenade(const std::string& fraction)
{
	if (fraction != _fraction)
	{
		TakeDamage(GetHealth(), "Grenade", fraction);
	}
}

void Tank::OnBonusStar(const std::string& author)
{
	if (author == _name)
	{
		SetHealth(GetHealth() + 50);
		if (_tier > 3)
		{
			return;
		}

		++_tier;

		_speed *= 1.10f;
		_calibre.speed *= 1.10f;
		_calibre.damage += 15;
		_calibre.damageRadius *= 1.25f;
		_calibre.tier = _tier;
		_shootTimer.cooldown -= milliseconds{150};

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_BonusStar_Pickup", author);
		}
	}
}

void Tank::OnBonusCaliber(const std::string& author)
{
	if (author == _name)
	{
		SetHealth(GetHealth() + 50);
		if (_tier > 3)
		{
			return;
		}

		_tier += 3;

		_speed *= 1.30f;
		_calibre.speed *= 1.30f;
		_calibre.damage += 45;
		_calibre.damageRadius *= 1.75f;
		_calibre.tier = _tier;
		_shootTimer.cooldown -= milliseconds{450};

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_BonusCaliber_Pickup", author);
		}
	}
}

void Tank::OnClientTankOnOff(const buuid uuid, const bool isEnable)
{
	if (uuid == _uuid)
	{
		isEnable ? Enable() : Disable();
	}
}

void Tank::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent("Statistics_TankHit", TankStatisticsEvent{_name, author, fraction});

	if (GetHealth() < 1)
	{
		//TODO: move to event from statistic when last tank died
		_events->EmitEvent("Statistics_TankDied", TankStatisticsEvent{_name, author, fraction});
	}
}

void Tank::HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const
{
	if (auto* bonus = dynamic_cast<IPickupableBonus*>(object.get()))
	{
		bonus->PickUpBonus(_name, _fraction);
	}
}

void Tank::OnClientChangePos(const FPoint newPos, const Direction dir, const buuid& uuid)
{
	if (uuid != _uuid)//TODO: check maybe never true
	{
		return;
	}

	SetDirection(dir);
	SetPos(newPos);

	//NOTE: fix for tank truck animation tick
	_events->EmitEvent("AnimationTankUpdate", AnimationTankUpdateEvent{GetName(), newPos, dir});
}

bool Tank::IsTouchBush() const
{
	auto bushCollisionsFilter = *_allObjects | std::views::filter([this](const std::shared_ptr<BaseObj>& object)
	{
		return _uuid != object->GetUuid()
			   && ColliderUtils::IsCollide(_rect, object->GetRect())
			   && dynamic_cast<BushTile*>(object.get()) != nullptr;
	});

	return !bushCollisionsFilter.empty();
}

bool Tank::IsTouchIce() const
{
	auto bushCollisionsFilter = *_allObjects | std::views::filter([this](const std::shared_ptr<BaseObj>& object)
	{
		return _uuid != object->GetUuid()
			   && ColliderUtils::IsCollide(_rect, object->GetRect())
			   && dynamic_cast<IceTile*>(object.get()) != nullptr;
	});

	return !bushCollisionsFilter.empty();
}

void Tank::ApplyScaleToCalibre(const float newScale)
{
	if (ColliderUtils::AreEqualAbsolute(newScale, 1))
	{
		return;
	}

	this->_calibre.speed *= newScale;
	this->_calibre.damageRadius *= newScale;
	this->_calibre.size.x *= newScale;
	this->_calibre.size.y *= newScale;
}
