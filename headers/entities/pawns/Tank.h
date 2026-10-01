#pragma once

#include "../BonusEffectProperty.h"
#include "Pawn.h"
#include "entities/BulletCaliber.h"
#include "utils/Timer.h"
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

enum class Faction : char8_t;
enum class Direction : char8_t;
enum class TankModel : char8_t;
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
	//NOTE: which tank it is, apart from who drives it - its numbers and its atlas row
	TankModel _model{};
	//NOTE: where the driver asked to go on the last tick, empty when it asked for nothing at all
	std::optional<Direction> _drivingTo{};

	void EmitMoved() const;

	//NOTE: a tank ahead gives way instead of standing like a wall - it is shoved along our own heading
	//before we move, and what is left of the room is all our own step gets
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

	//NOTE: shares of the model's own base - a flat +15 damage was a scout's whole shell and a fifth of a heavy's
	struct TierGrowth final
	{
		double speedShare{};
		double damageShare{};
		double blastShare{};
		double reloadShare{};
	};

	//NOTE: the numbers the player's tank used to gain per star, read back as shares of its own base
	static constexpr TierGrowth kTierStep{.speedShare = 0.10,
										  .damageShare = 1.0,
										  .blastShare = 0.25,
										  .reloadShare = 0.30};

	//NOTE: star and caliber differ only in how far they carry a tank along the same four tiers
	static constexpr unsigned short kStarTiers{1u};
	static constexpr unsigned short kCaliberTiers{3u};

	//NOTE: the top tier itself, not the last one that may still be upgraded - three stars reach it
	static constexpr unsigned short kMinTier{1u};
	static constexpr unsigned short kMaxTier{4u};
	//NOTE: healed by the pickup, as a share of the health the model was built with - half of it, as before
	static constexpr double kUpgradeHealShare{0.5};

	void Upgrade(unsigned short tiers);

	//NOTE: the model's own numbers, before a single tier has been earned
	[[nodiscard]] BulletCaliber BaseCaliber() const;

	//NOTE: tier N is what N-1 stars make, counted off the base - so spawning at a tier and earning it cannot drift
	void ApplyTier(unsigned short tier);
	void OnBonusStar();
	void OnBonusCaliber();
	void OnBonusShip();

protected:
	BulletCaliber _caliber{};
	Timer _shootTimer{};

	void EmitDamageStatistics(Author author) override;
	void EmitDeathStatistics(Author author) override;
	void OnDespawned(const DespawnedEvent& event) override;

	void Subscribe() override;
	void TickUpdate(double deltaTime) override;

	BonusEffectProperty _effects{};

	void Shot(std::optional<Uuid> withUuid = std::nullopt, std::optional<unsigned int> withDamage = std::nullopt);

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
	[[nodiscard]] TankModel GetModel() const noexcept;

	//NOTE: how far this tank gives way, the ones behind it counted in - a wall, the edge of the field or
	//a tank driving the other way ends the chain, and then the whole of it stands
	[[nodiscard]] double ShoveDistance(Direction dir, double wanted, int depth) const;
	//NOTE: leaning in, not just facing - a parked tank gives way, one pressing towards us holds the chain
	[[nodiscard]] bool IsDrivingAgainst(Direction dir) const;

	//NOTE: two tanks can lean on a third - alreadyMoved makes it give way once
	void ShoveBy(double distance, Direction dir, double velocity, std::vector<const Tank*>& alreadyMoved);

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
