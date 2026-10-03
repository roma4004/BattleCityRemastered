#include "entities/pawns/Tank.h"
#include "application/GameConfig.h"
#include "components/WorldGeometry.h"
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
#include "components/WorldSnapshot.h"
#include "entities/BulletCaliber.h"
#include "entities/TankModelSpec.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/TankResetProperty.h"
#include "geometry/Point.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/TankType.h"
#include "enums/Terrain.h"
#include "interfaces/IInputProvider.h"
#include "interfaces/IMoveBeh.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "utils/MathUtils.h"
#include "enums/Faction.h"
#include <algorithm>
#include <chrono>
#include <ranges>

using namespace std::chrono_literals;

namespace
{
//NOTE: the player's gun, and every other model is this one times its own share
constexpr auto kBaseReload{500ms};
}//namespace

Tank::Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
		   std::unique_ptr<IInputProvider> inputProvider, const GameConfig& gameConfig)
	: Pawn{std::move(pawnProperty), gameConfig, kCollision}
	, _inputProvider{std::move(inputProvider)}
{
	auto moveBeh{std::make_unique<MoveLikeTankBeh>(_rect, _speed, _uuid, _effects, gameConfig)};
	_tankMoveBeh = moveBeh.get();
	_moveBeh = std::move(moveBeh);
	ApplyFreshLoadout();
	//NOTE: the loadout is the first tier's, so a tank built above it is grown to its own
	ApplyTier(_tier);
	_shootingBeh = std::make_shared<ShootingBeh>(_rect, _dir, _uuid, _author, bulletPool, _caliber, _events,
												 _gameConfig);
}

void Tank::Activate()
{
	Pawn::Activate();
	_inputProvider->Enable();
}

void Tank::Deactivate()
{
	_inputProvider->Disable();
	Pawn::Deactivate();
}

BulletCaliber Tank::BaseCaliber() const
{
	constexpr FPoint baseShell{.x = 9.0, .y = 9.0};
	constexpr double baseBulletSpeed{300.0};
	constexpr unsigned int baseDamage{15};
	const FPoint shell{ShellSizeOf(_model, baseShell)};

	return BulletCaliber{.speed = baseBulletSpeed * SpecOf(_model).bulletSpeedFactor,
						 .damage = DamageOf(_model, baseDamage),
						 .damageRadius = BlastOf(_model, WorldGeometry::BlastRadiusFor(_gameConfig.tankSize,
																					   shell.y)),
						 .tier = _tier,
						 .size = shell};
}

//NOTE: the tier is not touched here - a fresh tank gets it from its property, a reused one from Reset
void Tank::ApplyFreshLoadout()
{
	_caliber = BaseCaliber();
	_effects = BonusEffectProperty{};
	_shootTimer = Timer{};
	_shootTimer.cooldown = ReloadOf(_model, kBaseReload);
}

void Tank::Reset(const TankResetProperty& resetProperty, std::unique_ptr<IInputProvider> driver)
{
	_uuid = resetProperty.uuid;
	_rect = resetProperty.rect;
	SetHealth(resetProperty.health);
	_dir = resetProperty.dir;

	_type = resetProperty.type;
	_model = resetProperty.model;
	_author = SeatOf(_type);
	_faction = FactionOf(_author, _gameConfig.Rules());
	_tier = 1u;

	ApplyFreshLoadout();
	ApplyTier(resetProperty.tier);

	_tankMoveBeh->ResetVelocity();

	//NOTE: not enabled here - a reset tank is not in the world yet, Activate is what puts it on the bus
	_inputProvider = std::move(driver);

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

	if (_gameConfig.IsClient())
	{
		SubscribeAsClient();
	}

	_subs.push_back(_events->AddListener(Key(_uuid), this, &Tank::OnBonusTimerReApplyOnSpawn));

	SubscribeBonus();

	//NOTE: whoever decides, not only a host - a local match answers the same question when a level ends
	if (_gameConfig.IsAuthority())
	{
		_subs.push_back(_events->AddListener(this, &Tank::OnWorldSnapshotRequested));
	}
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
	Shot(event.bulletUuid, event.damage);
}

void Tank::OnBonusHelmetApplied(const BonusHelmetAppliedEvent& event) { OnBonusHelmet(event.isActive); }

void Tank::OnTierChanged(const TierChangedEvent& event) { _tier = event.tier; }

void Tank::OnBonusShipApplied(const BonusShipAppliedEvent&) { OnBonusShip(); }

void Tank::SubscribeBonus()
{
	_subs.push_back(_events->AddListener(Key(_faction), this, &Tank::OnBonusTimer));
	_subs.push_back(_events->AddListener(Key(_faction), this, &Tank::OnBonusGrenade));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusHelmetStatusChange));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusStarPickup));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusCaliberPickup));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusShipPickup));
}

void Tank::OnBonusHelmetStatusChange(const BonusHelmetStatusChangeEvent& event) { OnBonusHelmet(event.isActive); }

void Tank::OnBonusStarPickup(const BonusStarPickupEvent&) { OnBonusStar(); }

void Tank::OnBonusCaliberPickup(const BonusCaliberPickupEvent&) { OnBonusCaliber(); }

void Tank::OnBonusShipPickup(const BonusShipPickupEvent&) { OnBonusShip(); }

void Tank::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	event.snapshot.tanks.push_back(TankSnapshot{.type = _type,
												.model = _model,
												.uuid = _uuid,
												.pos = GetPos(),
												.dir = _dir,
												.health = GetHealth(),
												.tier = _tier,
												.isHelmetActive = _effects.isHelmetActive,
												.isShipActive = _effects.isShipActive});
}

void Tank::TakeDamage(const unsigned int damage, const Author author)
{
	if (!_effects.isHelmetActive)
	{
		Pawn::TakeDamage(damage, author);
	}
}

unsigned int Tank::GetTier() const noexcept { return _tier; }

TankModel Tank::GetModel() const noexcept { return _model; }

bool Tank::CanShoot() const noexcept { return !_shootTimer.isActive; }

std::vector<Direction> Tank::GetFreePathSides(const double deltaTime,
											 const std::optional<Direction> excludeDirection) const
{
	return _tankMoveBeh->GetFreePathSides(deltaTime, excludeDirection, _allObjects);
}

void Tank::EmitMoved() const
{
	const FPoint pos{GetPos()};
	_events->EmitEvent(AnimationTankUpdateEvent{.author = _author, .pos = pos, .dir = _dir, .tier = _tier});

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(PosChangedEvent{.pos = pos, .dir = _dir, .uuid = _uuid});
	}
}

//NOTE: half of the step off ice, and that is the whole cost of pushing - what we shove the tank ahead by is the
//room our own move is then clamped to, so a tank with a tank on its nose travels half as far
constexpr double kShoveShare{0.5};

//NOTE: a chain longer than the tanks in a match cannot happen, and a cycle in it must not hang the frame
constexpr int kMaxShoveChain{8};

bool Tank::IsDrivingAgainst(const Direction dir) const
{
	return _drivingTo == DirectionUtils::Opposite(dir);
}

double Tank::ShoveDistance(const Direction dir, const double wanted, const int depth) const
{
	if (depth > kMaxShoveChain
		|| !DirectionUtils::FitsBeforeEdge(GetRect(), _gameConfig.battlefieldSize, wanted, dir))
	{
		return 0.0;
	}

	double allowed{wanted};
	for (const std::shared_ptr<BaseObj>& blocker: _tankMoveBeh->BlockersAhead(dir, wanted, _allObjects))
	{
		const auto peer{std::dynamic_pointer_cast<Tank>(blocker)};
		if (peer == nullptr || peer->IsDrivingAgainst(dir))
		{
			return 0.0;
		}

		allowed = std::min(allowed, peer->ShoveDistance(dir, wanted, depth + 1));
	}

	return allowed;
}

void Tank::ShoveBy(const double distance, const Direction dir, const double velocity,
				   std::vector<const Tank*>& alreadyMoved)
{
	if (std::ranges::find(alreadyMoved, this) != alreadyMoved.end())
	{
		return;
	}

	alreadyMoved.push_back(this);

	for (const std::shared_ptr<BaseObj>& blocker: _tankMoveBeh->BlockersAhead(dir, distance, _allObjects))
	{
		if (const auto peer{std::dynamic_pointer_cast<Tank>(blocker)})
		{
			peer->ShoveBy(distance, dir, velocity, alreadyMoved);
		}
	}

	if (_effects.isTouchTheIce)
	{
		_tankMoveBeh->Carry(dir, distance, velocity);
		EmitMoved();

		return;
	}

	const ObjRectangle moved{DirectionUtils::Moved(GetRect(), distance, dir)};
	SetPos(FPoint{.x = moved.x, .y = moved.y});
	EmitMoved();
}

//NOTE: the whole chain moves by one distance or none of it moves - a tank that cannot give way is a wall
//again, and so is one driving at us
void Tank::ShoveAhead(const Direction dir, const double step)
{
	const std::vector<std::shared_ptr<BaseObj>> blockers{_tankMoveBeh->BlockersAhead(dir, step, _allObjects)};
	if (blockers.empty())
	{
		return;
	}

	const double wanted{_effects.isTouchTheIce ? step : step * kShoveShare};
	double allowed{wanted};
	std::vector<std::shared_ptr<Tank>> pushed{};
	for (const std::shared_ptr<BaseObj>& blocker: blockers)
	{
		const auto peer{std::dynamic_pointer_cast<Tank>(blocker)};
		if (peer == nullptr || peer->IsDrivingAgainst(dir))
		{
			return;
		}

		allowed = std::min(allowed, peer->ShoveDistance(dir, wanted, 1));
		pushed.push_back(peer);
	}

	if (allowed <= 0.0)
	{
		return;
	}

	//NOTE: shared across the whole push, so the far end of the chain moves by one distance and no more
	std::vector<const Tank*> alreadyMoved{};
	const double velocity{_tankMoveBeh->GetVelocity(dir)};
	std::ranges::for_each(pushed, [allowed, dir, velocity, &alreadyMoved](const std::shared_ptr<Tank>& peer)
	{
		peer->ShoveBy(allowed, dir, velocity, alreadyMoved);
	});
}

//NOTE: the same loop whoever drives - the driver only answers where to go and whether to fire
void Tank::TickUpdate(const double deltaTime)
{
	if (_shootTimer.isActive && _shootTimer.IsCooldownFinish())
	{
		_shootTimer.isActive = false;
	}

	std::vector<std::shared_ptr<BaseObj>> outCollisions;
	const Direction oldDir{_dir};

	const std::optional<Direction> chosen{_inputProvider->ChooseDirection(*this, deltaTime)};
	_drivingTo = chosen;
	bool isMove{};
	if (chosen)
	{
		SetDirection(*chosen);
		ShoveAhead(*chosen, _speed * deltaTime);
		isMove = _moveBeh->Move(*chosen, deltaTime, _allObjects, outCollisions);
	}

	//NOTE: only when a move was actually attempted - a player pressing nothing is not blocked
	if (chosen && !isMove)
	{
		if (const std::optional<Direction> revised{_inputProvider->ReviseWhenMoveBlocked(*this, deltaTime)})
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
		if (_tankMoveBeh->ApplyMoveVelocity(deltaTime, _allObjects))
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
	if (const bool isTouchTheIce{IsTouchIce()};
		_effects.isTouchTheIce != isTouchTheIce)
	{
		_effects.isTouchTheIce = isTouchTheIce;
		_tankMoveBeh->ResetVelocity();
	}

	if (_inputProvider->ShouldShoot(*this) && !_shootTimer.isActive)
	{
		Shot();
	}
}

void Tank::Shot(const std::optional<Uuid> withUuid, const std::optional<unsigned int> withDamage)
{
	const ShotResult shot{_shootingBeh->Shot(withUuid, withDamage)};

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(TankShotEvent{.who = _author,
										 .dir = GetDirection(),
										 .bulletUuid = shot.uuid,
										 .damage = shot.damage});
	}

	_shootTimer.Reset();
}

double Tank::GetBulletWidth() const noexcept { return _caliber.size.x; }

void Tank::SetBulletWidth(const double bulletWidth) { _caliber.size.x = bulletWidth; }

double Tank::GetBulletHeight() const noexcept { return _caliber.size.y; }

void Tank::SetBulletHeight(const double bulletHeight) { _caliber.size.y = bulletHeight; }

double Tank::GetBulletSpeed() const noexcept { return _caliber.speed; }

void Tank::SetBulletSpeed(const double bulletSpeed) { _caliber.speed = bulletSpeed; }

unsigned int Tank::GetBulletDamage() const noexcept { return _caliber.damage; }

void Tank::SetBulletDamage(const unsigned int bulletDamage) { _caliber.damage = bulletDamage; }

double Tank::GetBulletDamageRadius() const noexcept { return _caliber.damageRadius; }

void Tank::SetBulletDamageRadius(const double bulletDamageRadius) { _caliber.damageRadius = bulletDamageRadius; }

void Tank::OnBonusTimer(const BonusTimerStatusChangeEvent& event)
{
	if (event.isActive && event.spared != _author)
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

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(BonusHelmetAppliedEvent{.author = _author, .isActive = isActive});
	}
}

void Tank::OnBonusGrenade(const BonusGrenadePickupEvent& event)
{
	if (const int health{GetHealth()}; health > 0 && event.spared != _author)
	{
		TakeDamage(static_cast<unsigned int>(health), Author::None);
	}
}

void Tank::ApplyTier(const unsigned short tier)
{
	_tier = std::clamp(tier, kMinTier, kMaxTier);

	const auto steps{static_cast<double>(_tier - kMinTier)};
	const double faster{1.0 + kTierStep.speedShare * steps};
	const BulletCaliber base{BaseCaliber()};

	_speed = SpeedOf(_model, _gameConfig.tankSpeed) * faster;
	_caliber.speed = base.speed * faster;
	_caliber.damage = MathUtils::RoundTo<unsigned int>(base.damage * (1.0 + kTierStep.damageShare * steps));
	_caliber.damageRadius = base.damageRadius * (1.0 + kTierStep.blastShare * steps);
	_caliber.tier = _tier;
	_shootTimer.cooldown = std::chrono::round<std::chrono::milliseconds>(
			ReloadOf(_model, kBaseReload) * (1.0 - kTierStep.reloadShare * steps));
}

void Tank::Upgrade(const unsigned short tiers)
{
	if (_tier >= kMaxTier)
	{
		return;
	}

	ApplyTier(static_cast<unsigned short>(_tier + tiers));

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(TierChangedEvent{.tier = _tier, .uuid = _uuid});
	}
}

void Tank::OnBonusStar() { Upgrade(kStarTiers); }

void Tank::OnBonusCaliber() { Upgrade(kCaliberTiers); }

void Tank::OnBonusShip()
{
	if (_effects.isShipActive)
	{
		return;
	}

	_effects.isShipActive = true;

	if (_gameConfig.IsHost())
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

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::Destroyed});
	}
}

void Tank::OnDespawned(const DespawnedEvent& event)
{
	Pawn::OnDespawned(event);

	_events->EmitEvent(AnimationCreateTankExplosionEvent{.rect = _rect, .author = _author});
}

void Tank::HandleBonusPickUp(const std::shared_ptr<BaseObj>& object)
{
	auto* bonus{dynamic_cast<IPickupableBonus*>(object.get())};
	if (bonus == nullptr || !object->GetIsAlive())
	{
		return;
	}

	bonus->PickUpBonus(_author);
	Heal(kPickupHeal);
}

void Tank::OnPosChanged(const PosChangedEvent& event)
{
	Pawn::OnPosChanged(event);

	_effects.isTouchTheBushes = IsTouchBush();

	//NOTE: the client's only move, so the track animation has to advance from here
	_events->EmitEvent(AnimationTankUpdateEvent{.author = _author,
												.pos = event.pos,
												.dir = event.dir,
												.tier = _tier});
}

bool Tank::IsTouchBush() const
{
	auto bushCollisionsFilter{_allObjects | std::views::filter([this](const std::shared_ptr<BaseObj>& object)
	{
		return _uuid != object->GetUuid()
			   && ColliderUtils::IsCollide(_rect, object->GetRect())
			   && object->GetTerrain() == Terrain::Bush;
	})};

	return !bushCollisionsFilter.empty();
}

bool Tank::IsTouchIce() const
{
	auto bushCollisionsFilter{_allObjects | std::views::filter([this](const std::shared_ptr<BaseObj>& object)
	{
		return _uuid != object->GetUuid()
			   && ColliderUtils::IsCollide(_rect, object->GetRect())
			   && object->GetTerrain() == Terrain::Ice;
	})};

	return !bushCollisionsFilter.empty();
}
