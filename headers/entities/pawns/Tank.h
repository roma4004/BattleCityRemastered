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
struct UPoint;
class PlayerTest;
class IShootable;
class BulletPool;
class GameConfig;
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

class Tank : public Pawn
{
	friend class TankSpawner;

	using milliseconds = std::chrono::milliseconds;
	std::shared_ptr<IShootable> _shootingBeh{nullptr};
	// Whole-lifetime listeners registered directly in the constructor.
	std::vector<EventSubscription> _permanentSubs{};

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

	static constexpr unsigned short kMaxTier{3u};
	static constexpr int kUpgradeHeal{50};

	void Upgrade(const TierUpgrade& upgrade);
	void OnBonusStar();
	void OnBonusCaliber();
	void OnBonusShip();

protected:
	BulletCalibre _calibre{};
	Timer _shootTimer{};

	void EmitDamageStatistics(const std::string& author, Faction faction) override;
	void EmitDeathStatistics(const std::string& author, Faction faction) override;
	void OnDespawned(const DespawnedEvent& event) override;

	void Subscribe() override;

	// bonuses
	BonusEffectProperty _effects{};

	void Shot(std::optional<Uuid> withUuid = std::nullopt);

	void HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const;
	void OnPosChanged(const PosChangedEvent& event);
	[[nodiscard]] bool IsTouchBush() const;
	[[nodiscard]] bool IsTouchIce() const;

public:
	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

	Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const GameConfig& gameConfig);

	~Tank() override;


	//BaseObj overrides
	void TakeDamage(unsigned int damage, const std::string& author, Faction faction) override;

	[[nodiscard]] unsigned int GetTier() const;

	[[nodiscard]] double GetBulletWidth() const;
	void SetBulletWidth(double bulletWidth);

	[[nodiscard]] double GetBulletHeight() const;
	void SetBulletHeight(double bulletHeight);

	[[nodiscard]] double GetBulletSpeed() const;
	void SetBulletSpeed(double bulletSpeed);

	[[nodiscard]] unsigned int GetBulletDamage() const;
	void SetBulletDamage(unsigned int bulletDamage);

	[[nodiscard]] double GetBulletDamageRadius() const;
	void SetBulletDamageRadius(double bulletDamageRadius);
};
