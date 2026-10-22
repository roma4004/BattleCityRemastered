#pragma once

#include "../BonusEffectProperty.h"
#include "Pawn.h"
#include "components/Sprite.h"
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
struct TankShotEvent;
struct TierChangedEvent;
struct TankGrewEvent;
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
	bool _isPendingGrowth{};

	void EmitMoved();

	//NOTE: shoves a tank ahead along our heading before we move; our step gets only the room left after the shove
	void ShoveAhead(Direction dir, double step);
	void ApplyFreshLoadout();

	//NOTE: a seat changing hands in a running match - the hull keeps its health and its tier, only the driver changes
	void Handover(TankType type, std::unique_ptr<IInputProvider> driver);

	void SubscribeAsClient() override;
	void SubscribeBonus();
	void OnBonusTimerReApplyOnSpawn(const BonusTimerReApplyOnSpawnEvent& event);
	void OnTankShot(const TankShotEvent& event);
	void OnBonusHelmetApplied(const BonusHelmetAppliedEvent& event);
	void OnTierChanged(const TierChangedEvent& event);
	void OnTankGrew(const TankGrewEvent& event);
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
		double sizeShare{};
	};

	//NOTE: the numbers the player's tank used to gain per star, read back as shares of its own base
	static constexpr TierGrowth kTierStep{.speedShare = 0.10,
										  .damageShare = 1.0,
										  .blastShare = 0.25,
										  .reloadShare = 0.30,
										  .sizeShare = 1.0 / 9.0};

	//NOTE: star and caliber differ only in how far they carry a tank along the same four tiers
	static constexpr unsigned short kStarTiers{1u};
	static constexpr unsigned short kCaliberTiers{3u};

	//NOTE: the top tier itself, not the last one that may still be upgraded - six stars reach it
	static constexpr unsigned short kMinTier{1u};
	static constexpr unsigned short kMaxTier{7u};
	//NOTE: past it a star adds shells to the volley instead - a harder shell would be multiplied by their number
	static constexpr unsigned short kLastShellTier{3u};
	//NOTE: any bonus heals its taker with no ceiling; the surplus carries over to the next level
	static constexpr unsigned int kPickupHeal{50};

	void Upgrade(unsigned short tiers);
	//NOTE: grows to the tier's size only when there is room - grown into a wall, the tank would be stuck
	void GrowIfPending();

	//NOTE: the model's own numbers, before a single tier has been earned
	[[nodiscard]] BulletCaliber BaseCaliber() const;

	//NOTE: tier N is what N-1 stars make, counted off the base - so spawning at a tier and earning it cannot drift
	void ApplyTier(unsigned short tier);
	void OnBonusStar();
	void OnBonusCaliber();
	void OnBonusShip();

protected:
	BonusEffectProperty _effects{};
	unsigned int _trackSteps{};
	BulletCaliber _caliber{};
	Timer _shootTimer{};

	void EmitDamageStatistics(Author author) override;
	void EmitDeathStatistics(Author author) override;
	void OnDespawned(const DespawnedEvent& event) override;

	void Subscribe() override;
	void TickUpdate(double deltaTime) override;

	void Shot();

	void HandleBonusPickUp(BaseObj& object);
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

	[[nodiscard]] std::optional<Sprite> Look() const override;
	[[nodiscard]] std::optional<Sprite> Cover() const override;
	[[nodiscard]] std::optional<unsigned int> ShownHealth() const override;

	//NOTE: back into service from the pool - everything a previous life could have changed
	void Reset(const TankResetProperty& resetProperty, std::unique_ptr<IInputProvider> driver);

	[[nodiscard]] static double SizeOf(TankModel model, unsigned short tier, const GameConfig& gameConfig);

	[[nodiscard]] unsigned int GetTier() const noexcept;

	//NOTE: how far this tank and the ones beyond it can be shoved; a wall, the edge or a tank pushing back stops all
	[[nodiscard]] double ShoveDistance(Direction dir, double wanted, int depth) const;
	//NOTE: driving towards dir, not just facing it - a parked tank is shoved, one driving at us blocks the chain
	[[nodiscard]] bool IsDrivingAgainst(Direction dir) const;

	//NOTE: two tanks may push a third - alreadyMoved shoves it only once
	void ShoveBy(double distance, Direction dir, double velocity, std::vector<const Tank*>& alreadyMoved);

	//NOTE: what the driver needs of the tank it drives
	[[nodiscard]] bool CanShoot() const noexcept;
	[[nodiscard]] std::vector<Direction> GetFreePathSides(double deltaTime,
												  std::optional<Direction> excludeDirection) const;

	[[nodiscard]] double GetBulletWidth() const noexcept;
	[[nodiscard]] double GetBulletHeight() const noexcept;
	[[nodiscard]] double GetBulletSpeed() const noexcept;
	[[nodiscard]] double GetBulletBlastRadius() const noexcept;
};
