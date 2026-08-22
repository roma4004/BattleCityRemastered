#include "entities/pawns/Tank.h"
#include "application/GameConfig.h"
#include "behavior/MoveLikeTankBeh.h"
#include "behavior/ShootingBeh.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "entities/BulletCalibre.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/GameMode.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/ColliderUtils.h"
#include <ranges>

Tank::Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, GameConfig& gameConfig)
	: Pawn{std::move(pawnProperty), gameConfig, s_collision}
{
	_moveBeh = std::make_unique<MoveLikeTankBeh>(_rect, _dir, _speed, _uuid, _name, _fraction,
												 _allObjects, _effects, gameConfig);
	_calibre = BulletCalibre{.speed = 300.f,
							 .damage = 15,
							 .damageRadius = 18.f,
							 .tier = _tier,
							 .size{.x = 9.f, .y = 9.f}};
	ApplyScaleToCalibre(gameConfig.scaleFactor);

	_shootingBeh = std::make_shared<ShootingBeh>(_rect, _dir, _uuid, _name, _fraction, _allObjects, bulletPool,
												 _calibre, _events, _gameConfig);

	Tank::Subscribe();

	if (IsClient(_gameMode))
	{
		_permanentSubs.push_back(_events->AddListener(Key(_uuid), this, &Tank::OnPosChanged));
	}

	_permanentSubs.push_back(_events->AddListener(Key(_uuid), this, &Tank::OnBonusTimerReApplyOnSpawn));
}

void Tank::OnBonusTimerReApplyOnSpawn(const BonusTimerReApplyOnSpawnEvent& event)
{
	if (event.isEnabled)
	{
		UnsubscribeTickUpdate();
	}
	else
	{
		SubscribeTickUpdate();
	}
}

Tank::~Tank()
{
	_events->EmitEvent(TankDiedEvent{.uuid = _uuid});

	_events->EmitEvent(AnimationCreateTankExplosionEvent{.rect = _rect, .name = _name});
}

void Tank::Subscribe()
{
	Pawn::Subscribe();

	_subs.push_back(_events->AddListener(this, &Tank::OnPostDraw));
	_subs.push_back(_events->AddListener(this, &Tank::OnScaleFactorChangedTo));

	if (IsClient(_gameMode))
	{
		SubscribeAsClient();
	}

	SubscribeBonus();
}

void Tank::OnPostDraw(const PostDrawEvent&) const
{
	if (_effects.isHelmetActive || _effects.isTouchTheBushes)
	{
		return;
	}

	_events->EmitEvent(RenderHealthBarEvent{.rect = GetRect(), .health = GetHealth()});
}

void Tank::OnScaleFactorChangedTo(const ScaleFactorChangedToEvent& event) { ApplyScaleToCalibre(event.scale); }

void Tank::SubscribeAsClient()
{
	//TODO: move bot timers to handle outside bot tank
	_subs.push_back(_events->AddListener(Key(_name), this, &Tank::OnTankShot));
	_subs.push_back(_events->AddListener(Key(_name), this, &Tank::OnBonusHelmetApplied));
	_subs.push_back(_events->AddListener(Key(_name), this, &Tank::OnBonusStarApplied));
	_subs.push_back(_events->AddListener(Key(_name), this, &Tank::OnBonusCaliberApplied));
}

void Tank::OnTankShot(const TankShotEvent& event)
{
	SetDirection(event.dir);
	Shot(event.bulletUuid);
}

void Tank::OnBonusHelmetApplied(const BonusHelmetAppliedEvent& event)
{
	OnBonusHelmet(_name, event.isActive);
}

void Tank::OnBonusStarApplied(const BonusStarAppliedEvent&) { OnBonusStar(_name); }

void Tank::OnBonusCaliberApplied(const BonusCaliberAppliedEvent&) { OnBonusCaliber(_name); }

void Tank::SubscribeBonus()
{
	_subs.push_back(_events->AddListener(this, &Tank::OnBonusTimer));
	_subs.push_back(_events->AddListener(this, &Tank::OnBonusHelmetStatusChange));
	_subs.push_back(_events->AddListener(this, &Tank::OnBonusGrenade));
	_subs.push_back(_events->AddListener(this, &Tank::OnBonusStarPickup));
	_subs.push_back(_events->AddListener(this, &Tank::OnBonusCaliberPickup));
}

void Tank::OnBonusHelmetStatusChange(const BonusHelmetStatusChangeEvent& event)
{
	OnBonusHelmet(event.name, event.isActive);
}

void Tank::OnBonusStarPickup(const BonusStarPickupEvent& event) { OnBonusStar(event.author); }

void Tank::OnBonusCaliberPickup(const BonusCaliberPickupEvent& event) { OnBonusCaliber(event.author); }

void Tank::TakeDamage(const unsigned int damage, const std::string& author, const std::string& fraction)
{
	if (!_effects.isHelmetActive)
	{
		Pawn::TakeDamage(damage, author, fraction);
	}
}

unsigned int Tank::GetTier() const { return _tier; }

void Tank::Shot(const Uuid withUuid)
{
	const Uuid bulletUuid = _shootingBeh->Shot(withUuid);

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(TankShotEvent{.who = _name, .dir = GetDirection(), .bulletUuid = bulletUuid});
	}

	_shootTimer.Reset();
}

float Tank::GetBulletWidth() const { return _calibre.size.x; }

void Tank::SetBulletWidth(const float bulletWidth) { _calibre.size.x = bulletWidth; }

float Tank::GetBulletHeight() const { return _calibre.size.y; }

void Tank::SetBulletHeight(const float bulletHeight) { _calibre.size.y = bulletHeight; }

float Tank::GetBulletSpeed() const { return _calibre.speed; }

void Tank::SetBulletSpeed(const float bulletSpeed) { _calibre.speed = bulletSpeed; }

unsigned int Tank::GetBulletDamage() const { return _calibre.damage; }

void Tank::SetBulletDamage(const unsigned int bulletDamage) { _calibre.damage = bulletDamage; }

float Tank::GetBulletDamageRadius() const { return _calibre.damageRadius; }

void Tank::SetBulletDamageRadius(const float bulletDamageRadius) { _calibre.damageRadius = bulletDamageRadius; }

void Tank::OnBonusTimer(const BonusTimerStatusChangeEvent& event)
{
	if (event.fraction == _fraction)
	{
		if (event.isActive)
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

		_events->EmitEvent(AnimationBonusHelmetChangeEvent{.name = _name, .isEnable = isActive});

		if (IsHost(_gameMode))
		{
			_events->EmitEvent(BonusHelmetAppliedEvent{.name = _name, .isActive = isActive});
		}
	}
}

void Tank::OnBonusGrenade(const BonusGrenadePickupEvent& event)
{
	if (event.fraction != _fraction)
	{
		if (const int health = GetHealth(); health > 0)
		{
			TakeDamage(static_cast<unsigned int>(health), "Grenade", event.fraction);
		}
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

		if (IsHost(_gameMode))
		{
			_events->EmitEvent(BonusStarAppliedEvent{.name = author});
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

		if (IsHost(_gameMode))
		{
			_events->EmitEvent(BonusCaliberAppliedEvent{.name = author});
		}
	}
}

void Tank::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	_events->EmitEvent(StatisticsTankHitEvent{.who = _name, .author = author, .fraction = fraction});

	if (GetHealth() < 1)
	{
		//TODO: move to event from statistic when last tank died
		_events->EmitEvent(StatisticsTankDiedEvent{.who = _name, .author = author, .fraction = fraction});
	}
}

void Tank::HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const
{
	if (auto* bonus = dynamic_cast<IPickupableBonus*>(object.get()))
	{
		bonus->PickUpBonus(_name, _fraction);
	}
}

void Tank::OnPosChanged(const PosChangedEvent& event)
{
	SetDirection(event.dir);
	SetPos(event.pos);

	//NOTE: fix for tank truck animation tick
	_events->EmitEvent(AnimationTankUpdateEvent{.name = GetName(), .pos = event.pos, .dir = event.dir});
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
