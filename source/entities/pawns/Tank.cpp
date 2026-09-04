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
#include "entities/pawns/TankResetProperty.h"
#include "geometry/Point.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "interfaces/IInputProvider.h"
#include "interfaces/IMoveBeh.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/ColliderUtils.h"
#include "enums/Faction.h"
#include <ranges>

namespace
{
//NOTE: a tank fires at the rate of the seat it drives, and the seat is already in its faction
constexpr std::chrono::milliseconds kEnemySeatCooldown{1000};
constexpr std::chrono::milliseconds kPlayerSeatCooldown{500};
}//namespace

Tank::Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
		   std::unique_ptr<IInputProvider> inputProvider, const GameConfig& gameConfig)
	: Pawn{std::move(pawnProperty), gameConfig, kCollision}
	, _inputProvider{std::move(inputProvider)}
{
	_moveBeh = std::make_unique<MoveLikeTankBeh>(_rect, _speed, _uuid, _effects, gameConfig);
	ApplyFreshLoadout();
	_shootingBeh = std::make_shared<ShootingBeh>(_rect, _dir, _uuid, _author, bulletPool, _calibre, _events,
												 _gameConfig);

	_inputProvider->Enable();
}

//NOTE: the tier is not touched here - a fresh tank gets it from its property, a reused one from Reset
void Tank::ApplyFreshLoadout()
{
	_calibre = BulletCalibre{.speed = 300.0,
							 .damage = 15,
							 .damageRadius = 18.0,
							 .tier = _tier,
							 .size{.x = 9.0, .y = 9.0}};
	_effects = BonusEffectProperty{};
	_shootTimer = Timer{};
	_shootTimer.cooldown = _faction == Faction::EnemyTeam ? kEnemySeatCooldown : kPlayerSeatCooldown;
}

void Tank::Reset(const TankResetProperty& resetProperty, std::unique_ptr<IInputProvider> driver)
{
	_gameMode = _gameConfig.gameMode;

	_uuid = resetProperty.uuid;
	_rect = resetProperty.rect;
	SetHealth(resetProperty.health);
	_dir = resetProperty.dir;
	_speed = resetProperty.speed;

	_author = resetProperty.author;
	_faction = FactionOf(_author);
	_tier = 1u;

	ApplyFreshLoadout();

	if (auto* moveBeh = dynamic_cast<MoveLikeTankBeh*>(_moveBeh.get()))
	{
		moveBeh->ResetVelocity();
	}

	_inputProvider = std::move(driver);
	_inputProvider->Enable();

	_isAlive = true;
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

Tank::~Tank() = default;

void Tank::Subscribe()
{
	Pawn::Subscribe();

	_subs.push_back(_events->AddListener(this, &Tank::OnPostDraw));

	if (IsClient(_gameMode))
	{
		SubscribeAsClient();
	}

	_subs.push_back(_events->AddListener(Key(_uuid), this, &Tank::OnBonusTimerReApplyOnSpawn));

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

void Tank::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnTankShot));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusHelmetApplied));
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Tank::OnTierChanged));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusShipApplied));
}

void Tank::OnTankShot(const TankShotEvent& event)
{
	SetDirection(event.dir);
	Shot(event.bulletUuid);
}

void Tank::OnBonusHelmetApplied(const BonusHelmetAppliedEvent& event) { OnBonusHelmet(event.isActive); }

void Tank::OnTierChanged(const TierChangedEvent& event) { _tier = event.tier; }

void Tank::OnBonusShipApplied(const BonusShipAppliedEvent&) { OnBonusShip(); }

void Tank::SubscribeBonus()
{
	//NOTE: these land on a whole team, so the bus picks by faction instead of every tank comparing
	_subs.push_back(_events->AddListener(Key(_faction), this, &Tank::OnBonusTimer));
	_subs.push_back(_events->AddListener(Key(_faction), this, &Tank::OnBonusGrenade));
	//NOTE: these land on one seat, so the bus picks instead of every tank comparing
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusHelmetStatusChange));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusStarPickup));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusCaliberPickup));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusShipPickup));
}

void Tank::OnBonusHelmetStatusChange(const BonusHelmetStatusChangeEvent& event) { OnBonusHelmet(event.isActive); }

void Tank::OnBonusStarPickup(const BonusStarPickupEvent&) { OnBonusStar(); }

void Tank::OnBonusCaliberPickup(const BonusCaliberPickupEvent&) { OnBonusCaliber(); }

void Tank::OnBonusShipPickup(const BonusShipPickupEvent&) { OnBonusShip(); }

void Tank::TakeDamage(const unsigned int damage, const Author author)
{
	if (!_effects.isHelmetActive)
	{
		Pawn::TakeDamage(damage, author);
	}
}

unsigned int Tank::GetTier() const { return _tier; }

bool Tank::CanShoot() const { return !_shootTimer.isActive; }

std::vector<Direction> Tank::GetFreePathSides(const double deltaTime,
											  const std::optional<Direction> excludeDirection) const
{
	return _moveBeh->GetFreePathSides(deltaTime, excludeDirection, _allObjects);
}

void Tank::EmitMoved() const
{
	const FPoint pos = GetPos();
	_events->EmitEvent(AnimationTankUpdateEvent{.author = _author, .pos = pos, .dir = _dir});

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(PosChangedEvent{.pos = pos, .dir = _dir, .uuid = _uuid});
	}
}

//NOTE: one loop for every tank there is - the driver answers where to go and whether to fire, the
//tank does the rest the same way whoever is at the wheel
void Tank::TickUpdate(const double deltaTime)
{
	if (_shootTimer.isActive && _shootTimer.IsCooldownFinish())
	{
		_shootTimer.isActive = false;
	}

	std::vector<std::shared_ptr<BaseObj>> outCollisions;
	const Direction oldDir{_dir};

	const std::optional<Direction> chosen = _inputProvider->ChooseDirection(*this, deltaTime);
	bool isMove{false};
	if (chosen)
	{
		SetDirection(*chosen);
		isMove = _moveBeh->Move(*chosen, deltaTime, _allObjects, outCollisions);
	}

	//NOTE: only when a move was actually attempted - a player pressing nothing is not blocked
	if (chosen && !isMove)
	{
		if (const std::optional<Direction> revised = _inputProvider->ReviseWhenMoveBlocked(*this, deltaTime))
		{
			SetDirection(*revised);
		}
	}

	if (isMove || oldDir != _dir)
	{
		EmitMoved();
	}

	if (_effects.isTouchTheIce)
	{
		if (auto* moveBeh = dynamic_cast<MoveLikeTankBeh*>(_moveBeh.get());
			moveBeh && moveBeh->ApplyMoveVelocity(deltaTime, _allObjects))
		{
			EmitMoved();
		}
	}

	if (!outCollisions.empty())
	{
		HandleBonusPickUp(outCollisions.front());
		outCollisions.clear();
	}

	_effects.isTouchTheBushes = IsTouchBush();
	if (const bool isTouchTheIce = IsTouchIce();
		_effects.isTouchTheIce != isTouchTheIce)
	{
		_effects.isTouchTheIce = isTouchTheIce;
		if (auto* moveBeh = dynamic_cast<MoveLikeTankBeh*>(_moveBeh.get()))
		{
			moveBeh->ResetVelocity();
		}
	}

	if (_inputProvider->ShouldShoot(*this) && !_shootTimer.isActive)
	{
		Shot();
	}
}

void Tank::Shot(const std::optional<Uuid> withUuid)
{
	const Uuid bulletUuid = _shootingBeh->Shot(withUuid);

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(TankShotEvent{.who = _author, .dir = GetDirection(), .bulletUuid = bulletUuid});
	}

	_shootTimer.Reset();
}

double Tank::GetBulletWidth() const { return _calibre.size.x; }

void Tank::SetBulletWidth(const double bulletWidth) { _calibre.size.x = bulletWidth; }

double Tank::GetBulletHeight() const { return _calibre.size.y; }

void Tank::SetBulletHeight(const double bulletHeight) { _calibre.size.y = bulletHeight; }

double Tank::GetBulletSpeed() const { return _calibre.speed; }

void Tank::SetBulletSpeed(const double bulletSpeed) { _calibre.speed = bulletSpeed; }

unsigned int Tank::GetBulletDamage() const { return _calibre.damage; }

void Tank::SetBulletDamage(const unsigned int bulletDamage) { _calibre.damage = bulletDamage; }

double Tank::GetBulletDamageRadius() const { return _calibre.damageRadius; }

void Tank::SetBulletDamageRadius(const double bulletDamageRadius) { _calibre.damageRadius = bulletDamageRadius; }

void Tank::OnBonusTimer(const BonusTimerStatusChangeEvent& event)
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

void Tank::OnBonusHelmet(const bool isActive)
{
	_effects.isHelmetActive = isActive;

	_events->EmitEvent(AnimationBonusHelmetChangeEvent{.author = _author, .isEnable = isActive});

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(BonusHelmetAppliedEvent{.author = _author, .isActive = isActive});
	}
}

void Tank::OnBonusGrenade(const BonusGrenadePickupEvent&)
{
	if (const int health = GetHealth(); health > 0)
	{
		TakeDamage(static_cast<unsigned int>(health), Author::None);
	}
}

void Tank::Upgrade(const TierUpgrade& upgrade)
{
	Heal(kUpgradeHeal);

	if (_tier > kMaxTier)
	{
		return;
	}

	_tier += upgrade.tiers;

	_speed *= upgrade.speedFactor;
	_calibre.speed *= upgrade.speedFactor;
	_calibre.damage += upgrade.damage;
	_calibre.damageRadius *= upgrade.radiusFactor;
	_calibre.tier = _tier;
	_shootTimer.cooldown -= upgrade.cooldownCut;

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(TierChangedEvent{.tier = _tier, .uuid = _uuid});
	}
}

void Tank::OnBonusStar()
{
	constexpr TierUpgrade star{.tiers = 1u,
							   .speedFactor = 1.10,
							   .damage = 15,
							   .radiusFactor = 1.25,
							   .cooldownCut = milliseconds{150}};

	Upgrade(star);
}

void Tank::OnBonusCaliber()
{
	constexpr TierUpgrade caliber{.tiers = 3u,
								  .speedFactor = 1.30,
								  .damage = 45,
								  .radiusFactor = 1.75,
								  .cooldownCut = milliseconds{450}};

	Upgrade(caliber);
}

void Tank::OnBonusShip()
{
	if (_effects.isShipActive)
	{
		return;
	}

	_effects.isShipActive = true;

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(BonusShipAppliedEvent{.author = _author});
	}
}

void Tank::EmitDamageStatistics(const Author author)
{
	_events->EmitEvent(StatisticsTankHitEvent{.who = _author, .author = author});
}

void Tank::EmitDeathStatistics(const Author author)
{
	_events->EmitEvent(TankDiedEvent{.who = _author, .uuid = _uuid, .author = author});
	_events->EmitEvent(AnimationCreateTankExplosionEvent{.rect = _rect, .author = _author});

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::Destroyed});
	}
}

void Tank::OnDespawned(const DespawnedEvent& event)
{
	Pawn::OnDespawned(event);

	_events->EmitEvent(AnimationCreateTankExplosionEvent{.rect = _rect, .author = _author});
}

void Tank::HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const
{
	if (auto* bonus = dynamic_cast<IPickupableBonus*>(object.get()))
	{
		bonus->PickUpBonus(_author);
	}
}

void Tank::OnPosChanged(const PosChangedEvent& event)
{
	Pawn::OnPosChanged(event);

	_effects.isTouchTheBushes = IsTouchBush();

	//NOTE: fix for tank truck animation tick
	_events->EmitEvent(AnimationTankUpdateEvent{.author = _author, .pos = event.pos, .dir = event.dir});
}

bool Tank::IsTouchBush() const
{
	auto bushCollisionsFilter = _allObjects | std::views::filter([this](const std::shared_ptr<BaseObj>& object)
	{
		return _uuid != object->GetUuid()
			   && ColliderUtils::IsCollide(_rect, object->GetRect())
			   && dynamic_cast<BushTile*>(object.get()) != nullptr;
	});

	return !bushCollisionsFilter.empty();
}

bool Tank::IsTouchIce() const
{
	auto bushCollisionsFilter = _allObjects | std::views::filter([this](const std::shared_ptr<BaseObj>& object)
	{
		return _uuid != object->GetUuid()
			   && ColliderUtils::IsCollide(_rect, object->GetRect())
			   && dynamic_cast<IceTile*>(object.get()) != nullptr;
	});

	return !bushCollisionsFilter.empty();
}
