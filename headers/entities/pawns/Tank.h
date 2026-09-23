#pragma once

#include "../BonusEffectProperty.h"
#include "Pawn.h"
#include "entities/BulletCalibre.h"
#include "utils/Timer.h"
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

enum class Faction : char8_t;
enum class Direction : char8_t;
enum class TankType : char8_t;
struct UPoint;
struct TankResetProperty;
struct PosChangedEvent;
struct BonusTimerReApplyOnSpawnEvent;
struct PostDrawEvent;
struct TankShotEvent;
struct TierChangedEvent;
struct BonusHelmetAppliedEvent;
struct BonusShipAppliedEvent;
struct BonusTimerStatusChangeEvent;
struct BonusHelmetStatusChangeEvent;
struct BonusGrenadePickupEvent;
struct BonusStarPickupEvent;
struct BonusCaliberPickupEvent;
struct BonusShipPickupEvent;
struct WorldSnapshotRequestedEvent;
class IInputProvider;
class MoveLikeTankBeh;
class IShootable;
class BulletPool;
class GameConfig;

class Tank final : public Pawn
{
	friend class TankSpawner;

	using milliseconds = std::chrono::milliseconds;
	std::shared_ptr<IShootable> _shootingBeh{nullptr};
	//NOTE: the object Pawn::_moveBeh owns, typed - set once in the constructor, never replaced
	MoveLikeTankBeh* _tankMoveBeh{nullptr};
	std::unique_ptr<IInputProvider> _inputProvider{nullptr};
	//NOTE: what the seat was spawned as - a client rebuilding the field needs it, the seat alone cannot tell
	TankType _type{};

	void EmitMoved() const;

	//NOTE: a tank ahead gives way instead of standing like a wall - it is shoved along our own heading
	//before we move, and what is left of the room is all our own step gets, so pushing is the slower way
	void ShoveAhead(Direction dir, double step);
	void ApplyFreshLoadout();

	void SubscribeAsClient() override;
	void SubscribeBonus();
	void OnBonusTimerReApplyOnSpawn(const BonusTimerReApplyOnSpawnEvent& event);
	void OnPostDraw(const PostDrawEvent&) const;
	void OnTankShot(const TankShotEvent& event);
	void OnBonusHelmetApplied(const BonusHelmetAppliedEvent& event);
	void OnTierChanged(const TierChangedEvent& event);
	void OnBonusShipApplied(const BonusShipAppliedEvent&);
	void OnBonusHelmetStatusChange(const BonusHelmetStatusChangeEvent& event);
	void OnBonusStarPickup(const BonusStarPickupEvent& event);
	void OnBonusCaliberPickup(const BonusCaliberPickupEvent& event);
	void OnBonusShipPickup(const BonusShipPickupEvent& event);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;

	void OnBonusTimer(const BonusTimerStatusChangeEvent& event);
	void OnBonusHelmet(bool isActive);

	void OnBonusGrenade(const BonusGrenadePickupEvent& event);

	//NOTE: star and caliber are the same upgrade with different numbers
	struct TierUpgrade
	{
		unsigned short tiers{};
		double speedFactor{};
		unsigned int damage{};
		double radiusFactor{};
		std::chrono::milliseconds cooldownCut{};
	};

	//NOTE: star and caliber differ only in how far they carry a tank along the same four tiers
	static constexpr TierUpgrade kStar{.tiers = 1u,
									   .speedFactor = 1.10,
									   .damage = 15,
									   .radiusFactor = 1.25,
									   .cooldownCut = std::chrono::milliseconds{150}};
	static constexpr TierUpgrade kCaliber{.tiers = 3u,
										  .speedFactor = 1.30,
										  .damage = 45,
										  .radiusFactor = 1.75,
										  .cooldownCut = std::chrono::milliseconds{450}};

	//NOTE: the top tier itself, not the last one that may still be upgraded - three stars reach it
	static constexpr unsigned short kMaxTier{4u};
	static constexpr int kUpgradeHeal{50};

	void Upgrade(const TierUpgrade& upgrade);

	//NOTE: the numbers of one upgrade without the heal and without telling the wire - what Reset replays
	void ApplyTierStep(const TierUpgrade& upgrade);

	//NOTE: a tank spawned at tier N is the tank N-1 stars would have made, stats and all
	void ApplyTier(unsigned short tier);
	void OnBonusStar();
	void OnBonusCaliber();
	void OnBonusShip();

protected:
	BulletCalibre _calibre{};
	Timer _shootTimer{};

	void EmitDamageStatistics(Author author) override;
	void EmitDeathStatistics(Author author) override;
	void OnDespawned(const DespawnedEvent& event) override;

	void Subscribe() override;
	void TickUpdate(double deltaTime) override;

	BonusEffectProperty _effects{};

	void Shot(std::optional<Uuid> withUuid = std::nullopt);

	void HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const;
	void OnPosChanged(const PosChangedEvent& event) override;
	[[nodiscard]] bool IsTouchBush() const;
	[[nodiscard]] bool IsTouchIce() const;

public:
	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{},
											  tags::NoTerrain{}};

	Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
		 std::unique_ptr<IInputProvider> inputProvider, const GameConfig& gameConfig);

	~Tank() override;

	void Activate() override;
	void Deactivate() override;

	void TakeDamage(unsigned int damage, Author author) override;

	//NOTE: back into service from the pool - everything a previous life could have changed
	void Reset(const TankResetProperty& resetProperty, std::unique_ptr<IInputProvider> driver);

	[[nodiscard]] unsigned int GetTier() const noexcept;

	//NOTE: how far this tank gives way, the ones behind it counted in - a wall, the edge of the field or
	//a tank driving the other way ends the chain, and then the whole of it stands
	[[nodiscard]] double ShoveDistance(Direction dir, double wanted, int depth) const;

	//NOTE: moves this tank and whatever it is pushing, and says so - a shoved tank does not move in its
	//own TickUpdate, so nothing else would tell the client or the animation where it went
	//NOTE: the chain is a graph, not a line - two tanks can both lean on a third, and it gives way once
	void ShoveBy(double distance, Direction dir, std::vector<const Tank*>& alreadyMoved);

	//NOTE: what the driver needs of the tank it drives
	[[nodiscard]] bool CanShoot() const noexcept;
	[[nodiscard]] std::vector<Direction> GetFreePathSides(double deltaTime,
												  std::optional<Direction> excludeDirection) const;

	[[nodiscard]] double GetBulletWidth() const noexcept;
	void SetBulletWidth(double bulletWidth);

	[[nodiscard]] double GetBulletHeight() const noexcept;
	void SetBulletHeight(double bulletHeight);

	[[nodiscard]] double GetBulletSpeed() const noexcept;
	void SetBulletSpeed(double bulletSpeed);

	[[nodiscard]] unsigned int GetBulletDamage() const noexcept;
	void SetBulletDamage(unsigned int bulletDamage);

	[[nodiscard]] double GetBulletDamageRadius() const noexcept;
	void SetBulletDamageRadius(double bulletDamageRadius);
};
