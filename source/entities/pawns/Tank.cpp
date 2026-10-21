#include "entities/pawns/Tank.h"
#include "application/GameConfig.h"
#include "components/WorldGeometry.h"
#include "behavior/MoveLikeTankBeh.h"
#include "behavior/ShootingBeh.h"
#include "components/BulletPool.h"
#include "components/Sprite.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "entities/BulletCaliber.h"
#include "entities/TankModelSpec.h"
#include "entities/obstacles/Obstacle.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/TankResetProperty.h"
#include "geometry/Point.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/DrawLayer.h"
#include "enums/GameMode.h"
#include "enums/TankType.h"
#include "enums/Terrain.h"
#include "enums/TextureType.h"
#include "interfaces/IInputProvider.h"
#include "interfaces/IMoveBeh.h"
#include "interfaces/IPickupableBonus.h"
#include "network/commands/WorldSnapshot.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "utils/MathUtils.h"
#include "utils/TimeUtils.h"
#include "utils/Uuid.h"
#include "enums/Faction.h"
#include <algorithm>
#include <chrono>
#include <optional>
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
	constexpr unsigned int baseDamage{30};
	constexpr unsigned int baseShellHealth{Obstacle::kWallHealth};
	const FPoint shell{ShellSizeOf(_model, baseShell)};

	return BulletCaliber{.speed = baseBulletSpeed * SpecOf(_model).bulletSpeedFactor,
						 .damage = DamageOf(_model, baseDamage),
						 .health = ShellHealthOf(_model, baseShellHealth),
						 .blastRadius = BlastOf(_model, WorldGeometry::BlastRadiusFor(_gameConfig.tankSize,
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
	_isPendingGrowth = false;

	ApplyFreshLoadout();
	ApplyTier(resetProperty.tier);

	_tankMoveBeh->ResetVelocity();

	//NOTE: not enabled here - a reset tank is not in the world yet, Activate is what puts it on the bus
	_inputProvider = std::move(driver);

	_isAlive = true;
}

//NOTE: the tank is in the world, so the new driver goes on the bus at once
void Tank::Handover(const TankType type, std::unique_ptr<IInputProvider> driver)
{
	_inputProvider->Disable();
	_inputProvider = std::move(driver);
	_inputProvider->Enable();
	_type = type;
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

std::optional<Sprite> Tank::Look() const
{
	constexpr unsigned int stepsPerFrame{2u};
	constexpr unsigned int frames{2u};

	return Sprite{.rect = _rect,
				  .layer = DrawLayer::World,
				  .dir = _dir,
				  .texture = TextureType::Tank,
				  .author = _author,
				  .model = _model,
				  .tier = _tier,
				  .frame = static_cast<int>(_trackSteps / stepsPerFrame % frames)};
}

//NOTE: the shimmer runs on the game clock, so a pause holds it
std::optional<Sprite> Tank::Cover() const
{
	if (!_effects.isHelmetActive)
	{
		return std::nullopt;
	}

	constexpr auto shimmerStep{std::chrono::milliseconds{333}};
	constexpr long long frames{2};
	const long long shimmer{TimeUtils::Now().time_since_epoch() / shimmerStep};

	return Sprite{.rect = _rect,
				  .layer = DrawLayer::Effects,
				  .texture = TextureType::Helmet,
				  .frame = static_cast<int>(shimmer % frames)};
}

//NOTE: none under a helmet or in a bush - the bush is meant to hide the tank, and the bar would give it away
std::optional<unsigned int> Tank::ShownHealth() const
{
	if (_effects.isHelmetActive || _effects.isTouchTheBushes)
	{
		return std::nullopt;
	}

	return GetHealth();
}

void Tank::SubscribeAsClient()
{
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnTankShot));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusHelmetApplied));
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Tank::OnTierChanged));
	_subs.push_back(_events->AddListener(Key(_uuid), this, &Tank::OnTankGrew));
	_subs.push_back(_events->AddListener(Key(_author), this, &Tank::OnBonusShipApplied));
}

void Tank::OnTankShot(const TankShotEvent& event)
{
	SetDirection(event.dir);
	_shootingBeh->Mirror(event.bulletUuid);
}

void Tank::OnBonusHelmetApplied(const BonusHelmetAppliedEvent& event) { OnBonusHelmet(event.isActive); }

void Tank::OnTierChanged(const TierChangedEvent& event)
{
	_tier = event.tier;
}

void Tank::OnTankGrew(const TankGrewEvent& event) { _rect = event.rect; }

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
	event.snapshot.tanks.push_back(network::commands::TankSnapshot{.type = _type,
												.model = _model,
												.uuid = _uuid,
												.rect = _rect,
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

bool Tank::CanShoot() const noexcept { return !_shootTimer.isActive; }

std::vector<Direction> Tank::GetFreePathSides(const double deltaTime,
											 const std::optional<Direction> excludeDirection) const
{
	return _tankMoveBeh->GetFreePathSides(deltaTime, excludeDirection, _allObjects);
}

void Tank::EmitMoved()
{
	const FPoint pos{GetPos()};
	++_trackSteps;

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
	for (const BaseObj* const blocker: _tankMoveBeh->BlockersAhead(dir, wanted, _allObjects))
	{
		const auto* const peer{dynamic_cast<const Tank*>(blocker)};
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

	for (BaseObj* const blocker: _tankMoveBeh->BlockersAhead(dir, distance, _allObjects))
	{
		if (auto* const peer{dynamic_cast<Tank*>(blocker)})
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
	const std::vector<BaseObj*> blockers{_tankMoveBeh->BlockersAhead(dir, step, _allObjects)};
	if (blockers.empty())
	{
		return;
	}

	const double wanted{_effects.isTouchTheIce ? step : step * kShoveShare};
	double allowed{wanted};
	std::vector<Tank*> pushed{};
	for (BaseObj* const blocker: blockers)
	{
		auto* const peer{dynamic_cast<Tank*>(blocker)};
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
	std::ranges::for_each(pushed, [allowed, dir, velocity, &alreadyMoved](Tank* const peer)
	{
		peer->ShoveBy(allowed, dir, velocity, alreadyMoved);
	});
}

//NOTE: the same loop whoever drives - the driver only answers where to go and whether to fire
void Tank::TickUpdate(const double deltaTime)
{
	GrowIfPending();

	if (_shootTimer.isActive && _shootTimer.IsCooldownFinish())
	{
		_shootTimer.isActive = false;
	}

	std::vector<BaseObj*> outCollisions;
	const Direction oldDir{_dir};

	const std::optional<Direction> chosen{_inputProvider->ChooseDirection(*this, deltaTime)};
	//NOTE: a turn takes the whole frame - a tap only aims the hull, the move starts on the next one
	const bool isTurn{chosen && *chosen != _dir};
	_drivingTo = isTurn ? std::nullopt : chosen;
	bool isMove{};
	if (isTurn)
	{
		SetDirection(*chosen);
	}
	else if (chosen)
	{
		ShoveAhead(*chosen, _speed * deltaTime);
		isMove = _moveBeh->Move(*chosen, deltaTime, _allObjects, outCollisions);
	}

	//NOTE: only when a move was actually attempted - a player pressing nothing is not blocked
	if (_drivingTo && !isMove)
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
		HandleBonusPickUp(*outCollisions.front());
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

void Tank::Shot()
{
	const std::vector<Uuid> shots{_shootingBeh->Volley(_allObjects)};

	if (_gameConfig.IsHost())
	{
		std::ranges::for_each(shots, [this](const Uuid shot)
		{
			_events->EmitEvent(TankShotEvent{.who = _author, .dir = GetDirection(), .bulletUuid = shot});
		});
	}

	_shootTimer.Reset();
}

double Tank::GetBulletWidth() const noexcept { return _caliber.size.x; }

double Tank::GetBulletHeight() const noexcept { return _caliber.size.y; }

double Tank::GetBulletSpeed() const noexcept { return _caliber.speed; }

double Tank::GetBulletBlastRadius() const noexcept { return _caliber.blastRadius; }

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

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(BonusHelmetAppliedEvent{.author = _author, .isActive = isActive});
	}
}

void Tank::OnBonusGrenade(const BonusGrenadePickupEvent& event)
{
	if (const unsigned int health{GetHealth()}; health > 0 && event.spared != _author)
	{
		TakeDamage(health, Author::None);
	}
}

void Tank::ApplyTier(const unsigned short tier)
{
	_tier = std::clamp(tier, kMinTier, kMaxTier);

	const auto steps{static_cast<double>(_tier - kMinTier)};
	const auto shellSteps{static_cast<double>(std::min(_tier, kLastShellTier) - kMinTier)};
	const double faster{1.0 + kTierStep.speedShare * steps};
	const BulletCaliber base{BaseCaliber()};

	_speed = SpeedOf(_model, _gameConfig.tankSpeed) * faster;
	_caliber.speed = base.speed * faster;
	_caliber.damage = MathUtils::RoundTo<unsigned int>(base.damage * (1.0 + kTierStep.damageShare * shellSteps));
	_caliber.health = MathUtils::RoundTo<unsigned int>(base.health * (1.0 + kTierStep.damageShare * shellSteps));
	_caliber.blastRadius = base.blastRadius * (1.0 + kTierStep.blastShare * shellSteps);
	_caliber.tier = _tier;
	_shootTimer.cooldown = std::chrono::round<std::chrono::milliseconds>(
			ReloadOf(_model, kBaseReload) * (1.0 - kTierStep.reloadShare * shellSteps));
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

	_isPendingGrowth = SizeOf(_model, _tier, _gameConfig) > _rect.w;
	GrowIfPending();
}

void Tank::GrowIfPending()
{
	if (!_isPendingGrowth || !_tankMoveBeh->TryGrow(SizeOf(_model, _tier, _gameConfig), _allObjects))
	{
		return;
	}

	_isPendingGrowth = false;
	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(TankGrewEvent{.rect = _rect, .uuid = _uuid});
	}
}

//NOTE: each tier above the first adds a ninth of the model's own size - a 3-cell tank grows to 4 cells at tier 4
double Tank::SizeOf(const TankModel model, const unsigned short tier, const GameConfig& gameConfig)
{
	const auto steps{static_cast<double>(std::clamp(tier, kMinTier, kMaxTier) - kMinTier)};
	const double side{gameConfig.gridOffset * static_cast<double>(SpecOf(model).cellSpan)
					  * (1.0 + kTierStep.sizeShare * steps)};

	return std::min(side, gameConfig.gridOffset * static_cast<double>(WorldGeometry::kMaxTankCellSpan));
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
	_events->EmitEvent(AnimationCreateTankExplosionEvent{.rect = _rect});

	if (_gameConfig.IsHost())
	{
		_events->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::Destroyed});
	}
}

void Tank::OnDespawned(const DespawnedEvent& event)
{
	Pawn::OnDespawned(event);

	_events->EmitEvent(AnimationCreateTankExplosionEvent{.rect = _rect});
}

void Tank::HandleBonusPickUp(BaseObj& object)
{
	auto* bonus{dynamic_cast<IPickupableBonus*>(&object)};
	if (bonus == nullptr || !object.GetIsAlive())
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

	//NOTE: the client's only move, so the track turns from here
	++_trackSteps;
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
