#pragma once

#include "../BonusEffectProperty.h"
#include "Pawn.h"
#include "entities/BulletCalibre.h"

struct UPoint;
class PlayerTest;
class IShootable;
class BulletPool;

class Tank : public Pawn
{
	friend class TankSpawner;

	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::shared_ptr<IShootable> _shootingBeh{nullptr};

	void SubscribeAsClient() override;
	void SubscribeBonus();

	void OnBonusTimer(const std::string& fraction, bool isActive);
	void OnBonusHelmet(const std::string& name, bool isActive);

	void OnBonusGrenade(const std::string& fraction);
	void OnBonusStar(const std::string& author);
	void OnBonusCaliber(const std::string& author);
	void OnClientTankOnOff(buuid uuid, bool isEnable);

protected:
	BulletCalibre _calibre{};
	milliseconds _fireCooldown{std::chrono::seconds{1}};
	mutable std::chrono::time_point<std::chrono::system_clock> _lastTimeFire{};

	void Subscribe() override;
	void Unsubscribe() const override;

	// bonuses
	BonusEffectProperty _effects{};

	void Shot(buuid withUuid = {}) const;

	void HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const;

	void TickUpdate(double deltaTime) override = 0;

	virtual void Enable();
	virtual void Disable() const;

public:
	Tank(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, bool enableByDefault = false);

	~Tank() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void TakeDamage(int damage) override;

	[[nodiscard]] int GetTier() const;

	[[nodiscard]] float GetBulletWidth() const;
	void SetBulletWidth(float bulletWidth);

	[[nodiscard]] float GetBulletHeight() const;
	void SetBulletHeight(float bulletHeight);

	[[nodiscard]] float GetBulletSpeed() const;
	void SetBulletSpeed(float bulletSpeed);

	[[nodiscard]] int GetBulletDamage() const;
	void SetBulletDamage(int bulletDamage);

	[[nodiscard]] double GetBulletDamageRadius() const;
	void SetBulletDamageRadius(double bulletDamageRadius);
};
