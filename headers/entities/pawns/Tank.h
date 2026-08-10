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

class Tank : public Pawn
{
	friend class TankSpawner;

	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::shared_ptr<IShootable> _shootingBeh{nullptr};
	// Whole-lifetime listeners registered directly in the constructor.
	std::vector<EventSubscription> _permanentSubs{};

	void SubscribeAsClient() override;
	void SubscribeBonus();

	void OnBonusTimer(const std::string& fraction, bool isActive);
	void OnBonusHelmet(const std::string& name, bool isActive);

	void OnBonusGrenade(const std::string& fraction);
	void OnBonusStar(const std::string& author);
	void OnBonusCaliber(const std::string& author);

protected:
	BulletCalibre _calibre{};
	Timer _shootTimer{};

	void Subscribe() override;

	// bonuses
	BonusEffectProperty _effects{};

	void Shot(buuid withUuid = {});

	void HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const;
	void OnClientChangePos(FPoint newPos, Direction dir);
	void ApplyScaleToCalibre(float newScale);
	[[nodiscard]] bool IsTouchBush() const;
	[[nodiscard]] bool IsTouchIce() const;

public:
	static constexpr CollisionTags kCollision{tags::Impassable{}, tags::Destructible{}, tags::Impenetrable{}};

	Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, GameConfig& gameConfig);

	~Tank() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	//BaseObj overrides
	void TakeDamage(unsigned int damage, const std::string& damageAuthor, const std::string& damageFraction) override;

	[[nodiscard]] unsigned int GetTier() const;

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	[[nodiscard]] unsigned int GetBulletDamage() const;
	void SetBulletDamage(unsigned int bulletDamage);

	[[nodiscard]] double GetBulletDamageRadius() const;
	void SetBulletDamageRadius(double bulletDamageRadius);
};
