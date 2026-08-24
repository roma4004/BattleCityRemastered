#pragma once

#include "../BonusEffectProperty.h"
#include "Pawn.h"
#include "entities/BulletCalibre.h"
#include "utils/Timer.h"

struct UPoint;
class PlayerTest;
class IShootable;
class BulletPool;
class GameConfig;
struct PosChangedEvent;
struct BonusTimerReApplyOnSpawnEvent;
struct PostDrawEvent;
struct ScaleFactorChangedToEvent;
struct TankShotEvent;
struct BonusHelmetAppliedEvent;
struct BonusStarAppliedEvent;
struct BonusCaliberAppliedEvent;
struct BonusTimerStatusChangeEvent;
struct BonusHelmetStatusChangeEvent;
struct BonusGrenadePickupEvent;
struct BonusStarPickupEvent;
struct BonusCaliberPickupEvent;

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
	void OnScaleFactorChangedTo(const ScaleFactorChangedToEvent& event);
	void OnTankShot(const TankShotEvent& event);
	void OnBonusHelmetApplied(const BonusHelmetAppliedEvent& event);
	void OnBonusStarApplied(const BonusStarAppliedEvent&);
	void OnBonusCaliberApplied(const BonusCaliberAppliedEvent&);
	void OnBonusHelmetStatusChange(const BonusHelmetStatusChangeEvent& event);
	void OnBonusStarPickup(const BonusStarPickupEvent& event);
	void OnBonusCaliberPickup(const BonusCaliberPickupEvent& event);

	void OnBonusTimer(const BonusTimerStatusChangeEvent& event);
	void OnBonusHelmet(const std::string& name, bool isActive);

	void OnBonusGrenade(const BonusGrenadePickupEvent& event);
	void OnBonusStar(const std::string& author);
	void OnBonusCaliber(const std::string& author);

protected:
	BulletCalibre _calibre{};
	Timer _shootTimer{};

	void EmitDamageStatistics(const std::string& author, const std::string& fraction) override;
	void EmitDeathStatistics(const std::string& author, const std::string& fraction) override;

	void Subscribe() override;

	// bonuses
	BonusEffectProperty _effects{};

	void Shot(Uuid withUuid = {});

	void HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const;
	void OnPosChanged(const PosChangedEvent& event);
	void ApplyScaleToCalibre(float newScale);
	[[nodiscard]] bool IsTouchBush() const;
	[[nodiscard]] bool IsTouchIce() const;

public:
	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

	Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, GameConfig& gameConfig);

	~Tank() override;


	//BaseObj overrides
	void TakeDamage(unsigned int damage, const std::string& author, const std::string& fraction) override;

	[[nodiscard]] unsigned int GetTier() const;

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	[[nodiscard]] unsigned int GetBulletDamage() const;
	void SetBulletDamage(unsigned int bulletDamage);

	[[nodiscard]] float GetBulletDamageRadius() const;
	void SetBulletDamageRadius(float bulletDamageRadius);
};
